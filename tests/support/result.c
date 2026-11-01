#include "tool.h"
#include "build_info.h"
#include "source_identity.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <yaml.h>

void tool_json_string(FILE *f, const char *text) {
    fputc('"', f);
    for (const unsigned char *p = (const unsigned char *)(text ? text : ""); *p; p++) {
        if (*p == '"' || *p == '\\') {
            fputc('\\', f);
            fputc(*p, f);
        } else if (*p < 32) {
            fprintf(f, "\\u%04x", *p);
        } else {
            fputc(*p, f);
        }
    }
    fputc('"', f);
}

static int compare_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static void write_optional_cpu(FILE *f, double value) {
    if (isfinite(value) && value >= 0) {
        fprintf(f, "%.6f", value);
    } else {
        fputs("null", f);
    }
}

static void write_optional_rss(FILE *f, uint64_t value) {
    if (value == UINT64_MAX) {
        fputs("null", f);
    } else {
        fprintf(f, "%llu", (unsigned long long)value);
    }
}

static void write_context(FILE *f, const ToolOptions *o) {
    fputs(",\n\"build\":{\"revision\":", f);
    tool_json_string(f, TOOL_REVISION);
    fputs(",\"chitra_revision\":", f);
    tool_json_string(f, TOOL_CHITRA_REVISION);
    fputs(",\"chitra_dirty_at_build\":", f);
    tool_json_string(f, TOOL_CHITRA_DIRTY);
    fputs(",\"tether_tools_source_sha256\":", f);
    tool_json_string(f, TOOL_SOURCE_SHA256);
    fputs(",\"dirty_at_build\":", f);
    tool_json_string(f, TOOL_SOURCE_DIRTY);
    fputs(",\"compiler\":", f);
    tool_json_string(f, TOOL_COMPILER);
    fputs(",\"configuration\":", f);
    tool_json_string(f, TOOL_BUILD_CONFIG);
    fprintf(f, ",\"address_sanitizer\":%s", !strcmp(TOOL_ASAN, "ON") ? "true" : "false");
    fputs(",\"cmake_c_flags\":", f);
    tool_json_string(f, TOOL_C_FLAGS);
    fputs(",\"configuration_c_flags\":", f);
    tool_json_string(f, !strcmp(TOOL_BUILD_CONFIG, "Debug")            ? TOOL_FLAGS_DEBUG
                        : !strcmp(TOOL_BUILD_CONFIG, "Release")        ? TOOL_FLAGS_RELEASE
                        : !strcmp(TOOL_BUILD_CONFIG, "RelWithDebInfo") ? TOOL_FLAGS_RELWITHDEBINFO
                                                                       : "unknown");
    fputs(",\"libyaml_version\":", f);
    tool_json_string(f, yaml_get_version_string());
    fputs(",\"build_system\":", f);
    tool_json_string(f, TOOL_SYSTEM);
    fprintf(f, ",\"pointer_bits\":%u,\"c_standard\":99},\n\"environment\":{",
            (unsigned)(sizeof(void *) * 8));
    tool_environment(f);
    fputs("},\n\"parameters\":{\"scenario\":", f);
    tool_json_string(f, o->scenario);
    fputs(",\"mode\":", f);
    tool_json_string(f, o->mode);
    fprintf(
        f,
        ",\"count\":%u,\"spawn_per_step\":%u,\"delete_per_step\":%u,\"mutate_per_step\":%u,"
        "\"frames\":%u,\"warmup\":%u,\"seconds\":%.6f,\"seed\":%u,\"memory_mib\":%u,\"fps\":%u},\n",
        o->count, o->spawn, o->remove, o->mutate, o->frames, o->warmup, o->seconds, o->seed,
        o->memory_mib, o->fps);
}

static void write_methodology(FILE *f, const ToolOptions *o, const ToolResult *r) {
    fputs("\"methodology\":{\"backend\":", f);
    tool_json_string(f, r->backend);
    bool rendered = !strcmp(o->scenario, "render") || !strcmp(o->scenario, "ui");
    fprintf(f,
            ",\"rendered\":%s,\"effective_warmup_frames\":%u,\"initial_window_width\":%s,\"initial_"
            "window_height\":%s",
            rendered ? "true" : "false", r->warmup_frames, rendered ? "1280" : "null",
            rendered ? "720" : "null");
    fputs(",\"counter_scope\":\"RHI draw_batch calls and vertex/index upload byte arguments; "
          "cumulative across measured steps including background; not GPU completion or transfer "
          "verification\","
          "\"phase_scope\":\"setup includes initial_create; shutdown includes teardown; validation "
          "overlaps elapsed for correctness tests; total_run excludes report writing; do not sum "
          "phases\","
          "\"population_scope\":\"alive counts workload entities; logical_visible counts visible "
          "panels, not rasterized pixels; hierarchy steps end empty and peak_active includes the "
          "temporary parent\"",
          f);
    fputs(",\"timing\":\"monotonic wall clock; ECS/render work includes mutations, engine "
          "update, preparation, and render submission; layout_ms includes only isolated "
          "tether_layout_process_all calls; excludes post-frame pacing and result writing\","
          "\"cpu\":\"process user plus kernel time during measured steps\","
          "\"memory\":\"process resident bytes; peak is sampled, not allocator or VRAM usage\","
          "\"workload\":\"fixed-seed colors; delete from tail, append new entities, mutate prefix; "
          "rendered panels occupy a fixed anchor grid based on maximum planned population; large "
          "grids become subpixel\","
          "\"render_policy\":\"force a backend presentation each measured render step in both "
          "modes; retained layout/visual invalidation remains selective\","
          "\"limitations\":\"one process/run; repeat explicitly; launcher and other processes may "
          "contend; short CPU measurements may round to zero; no GPU completion timestamps; no "
          "layout timing is isolated only in the layout scenario; hierarchy steps include "
          "assertions and full "
          "create/detach/destroy cycles; CPU scenarios ignore render mode and FPS; source "
          "identity captured at build, compiler settings at CMake configure\"},\n",
          f);
}

static void write_metrics(FILE *f, const ToolResult *r) {
    fprintf(f,
            "\"phases\":{\"setup_ms\":%.6f,\"warmup_ms\":%.6f,"
            "\"validation_ms\":%.6f,\"shutdown_ms\":%.6f,\"total_run_ms\":%.6f},\n",
            r->setup_ms, r->warmup_ms, r->validation_ms, r->shutdown_ms, r->total_run_ms);
    fprintf(f,
            "\"measurement_state\":{\"process_cpu\":\"%s\",\"resident_memory\":\"%s\","
            "\"gpu_time\":\"not_measured\",\"layout_time\":\"%s\"},\n",
            !r->sample_count      ? "not_measured"
            : isfinite(r->cpu_ms) ? "measured"
                                  : "query_failed",
            !r->sample_count            ? "not_measured"
            : r->rss_peak == UINT64_MAX ? "query_failed"
                                        : "measured",
            r->sample_count && r->backend && !strcmp(r->backend, "Tether layout") ? "measured"
                                                                                  : "not_measured");
    fprintf(f,
            "\"metrics\":{\"checks\":%llu,\"failures\":%llu,\"measured_steps\":%u,"
            "\"initial_create_ms\":%.6f,\"teardown_ms\":%.6f,\"elapsed_ms\":%.6f,"
            "\"process_cpu_ms\":",
            (unsigned long long)r->checks, (unsigned long long)r->failures, r->sample_count,
            r->create_ms, r->teardown_ms, r->elapsed_ms);
    write_optional_cpu(f, r->cpu_ms);
    fputs(",\"rss_start_bytes\":", f);
    write_optional_rss(f, r->rss_start);
    fputs(",\"rss_end_bytes\":", f);
    write_optional_rss(f, r->rss_end);
    fputs(",\"rss_sampled_peak_bytes\":", f);
    write_optional_rss(f, r->rss_peak);
    fputs(",\"gpu_ms\":null,\"layout_ms\":", f);
    if (r->sample_count && r->backend && !strcmp(r->backend, "Tether layout")) {
        fprintf(f, "%.6f", r->layout_ms);
    } else {
        fputs("null", f);
    }
    fprintf(f, ",\"initial_active\":%u,\"final_active\":%u,\"peak_active\":%u", r->initial_active,
            r->final_active, r->peak_active);
    double *times = r->sample_count ? malloc(sizeof(double) * r->sample_count) : NULL;
    if (times) {
        double sum = 0;
        for (uint32_t i = 0; i < r->sample_count; i++) {
            times[i] = r->samples[i].work_ms;
            sum += times[i];
        }
        qsort(times, r->sample_count, sizeof(double), compare_double);
        fprintf(f, ",\"work_mean_ms\":%.6f,\"work_p50_ms\":%.6f,\"work_p95_ms\":%.6f",
                sum / r->sample_count, times[(r->sample_count - 1) / 2],
                times[(size_t)ceil(0.95 * r->sample_count) - 1]);
        free(times);
    }
    bool rendered =
        r->backend && strcmp(r->backend, "none") != 0 && strcmp(r->backend, "Tether layout") != 0;
    if (rendered && r->sample_count > 1) {
        uint32_t intervals = r->sample_count - 1;
        double *frame_times = malloc(sizeof(double) * intervals);
        if (frame_times) {
            double total = 0;
            for (uint32_t i = 0; i < intervals; i++) {
                frame_times[i] = r->samples[i + 1].interval_ms;
                total += frame_times[i];
            }
            qsort(frame_times, intervals, sizeof(double), compare_double);
            fprintf(f, ",\"frame_interval_mean_ms\":%.6f,\"frame_interval_p95_ms\":%.6f",
                    total / intervals, frame_times[(size_t)ceil(0.95 * intervals) - 1]);
            free(frame_times);
        } else {
            fputs(",\"frame_interval_mean_ms\":null,\"frame_interval_p95_ms\":null", f);
        }
    } else {
        fputs(",\"frame_interval_mean_ms\":null,\"frame_interval_p95_ms\":null", f);
    }
    uint64_t draws = 0, uploads = 0;
    for (uint32_t i = 0; i < r->sample_count; i++) {
        draws += r->samples[i].draws;
        uploads += r->samples[i].upload_bytes;
    }
    fprintf(f, ",\"draw_calls_total\":%llu,\"upload_bytes_total\":%llu", (unsigned long long)draws,
            (unsigned long long)uploads);
}

static void write_samples(FILE *f, const ToolResult *r) {
    fputs("},\n\"samples\":[\n", f);
    for (uint32_t i = 0; i < r->sample_count; i++) {
        const ToolSample *s = &r->samples[i];
        fprintf(
            f,
            "%s{\"step\":%u,\"alive\":%u,\"spawned\":%u,\"deleted\":%u,\"mutated\":%u,\"work_ms\":%"
            ".6f,\"interval_ms\":%.6f,\"spawn_ms\":%.6f,\"delete_ms\":%.6f,\"mutate_ms\":%.6f,"
            "\"draw_calls\":%u,\"upload_bytes\":%llu,\"viewport_width\":%u,\"viewport_height\":%u,"
            "\"rss_bytes\":",
            i ? ",\n" : "", i, s->alive, s->spawned, s->deleted, s->mutated, s->work_ms,
            s->interval_ms, s->spawn_ms, s->delete_ms, s->mutate_ms, s->draws,
            (unsigned long long)s->upload_bytes, s->viewport_width, s->viewport_height);
        write_optional_rss(f, s->rss_bytes);
        fprintf(f, ",\"logical_visible\":%u,\"rendered_elements\":null}", s->visible);
    }
    fputs("\n]}\n", f);
}

bool tool_write_result(const ToolOptions *o, const ToolResult *r) {
    char temporary[4096];
    if (snprintf(temporary, sizeof temporary, "%s.%u.tmp", o->output, tool_process_id()) >=
        (int)sizeof temporary) {
        return false;
    }
    FILE *f = fopen(temporary, "wb");
    if (!f) {
        perror(o->output);
        return false;
    }
    char timestamp[40] = "unknown";
    time_t now = time(NULL);
    struct tm *utc = gmtime(&now);
    if (utc) {
        strftime(timestamp, sizeof timestamp, "%Y-%m-%dT%H:%M:%SZ", utc);
    }
    fprintf(f, "{\n\"schema_version\":1,\"project\":\"Tether\",\"kind\":\"%s\",\"status\":",
            o->test ? "correctness" : "benchmark");
    tool_json_string(f, r->status);
    fputs(",\"termination_reason\":", f);
    tool_json_string(f, r->termination_reason ? r->termination_reason : "not_finished");
    fputs(",\"summary\":", f);
    tool_json_string(f, r->message);
    fputs(",\"timestamp_utc\":", f);
    tool_json_string(f, timestamp);
    write_context(f, o);
    write_methodology(f, o, r);
    write_metrics(f, r);
    write_samples(f, r);
    bool ok = !ferror(f);
    if (fclose(f)) {
        ok = false;
    }
    if (ok) {
        ok = tool_replace_file(temporary, o->output);
    }
    if (!ok) {
        remove(temporary);
    }
    return ok;
}

bool tool_read_summary(const char *path, char *out, size_t size) {
    ToolReport report;
    if (!size || !tool_read_report_data(path, &report)) {
        return false;
    }
    int written = snprintf(out, size, "%s", report.value[REPORT_SUMMARY]);
    return written >= 0 && (size_t)written < size;
}
