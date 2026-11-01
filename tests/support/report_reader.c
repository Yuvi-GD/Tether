#include "tool.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

static const char *keys[REPORT_FIELD_COUNT] = {"status",
                                               "summary",
                                               "scenario",
                                               "mode",
                                               "count",
                                               "measured_steps",
                                               "checks",
                                               "failures",
                                               "elapsed_ms",
                                               "process_cpu_ms",
                                               "rss_sampled_peak_bytes",
                                               "work_mean_ms",
                                               "work_p50_ms",
                                               "work_p95_ms",
                                               "gpu_ms",
                                               "draw_calls_total",
                                               "upload_bytes_total",
                                               "kind",
                                               "frame_interval_mean_ms",
                                               "frame_interval_p95_ms",
                                               "layout_ms",
                                               "process_cpu",
                                               "resident_memory"};

/* Bounded stack and streaming parsing: no per-sample allocations are retained.
   The size limit bounds synchronous imports in the workbench. Larger artifacts
   remain usable by offline analysis tools. */
#define REPORT_MAX_BYTES (64L * 1024L * 1024L)
#define REPORT_MAX_DEPTH 8

typedef struct {
    yaml_event_type_t kind;
    bool expects_key;
    char key[128];
} ReportFrame;

static bool unsigned_value(const char *text, uint64_t *value) {
    if (!text[0]) {
        return false;
    }
    for (const char *p = text; *p; p++) {
        if (*p < '0' || *p > '9') {
            return false;
        }
    }
    errno = 0;
    char *end;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (errno || *end) {
        return false;
    }
    *value = (uint64_t)parsed;
    return true;
}

static bool nonnegative_value(const char *text) {
    errno = 0;
    char *end;
    double value = strtod(text, &end);
    return text[0] && end != text && !*end && !errno && isfinite(value) && value >= 0;
}

bool tool_read_report_data(const char *path, ToolReport *report) {
    memset(report, 0, sizeof *report);
    FILE *file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    if (fseek(file, 0, SEEK_END) || ftell(file) < 0 || ftell(file) > REPORT_MAX_BYTES ||
        fseek(file, 0, SEEK_SET)) {
        fclose(file);
        return false;
    }
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        fclose(file);
        return false;
    }
    yaml_parser_set_input_file(&parser, file);
    ReportFrame stack[REPORT_MAX_DEPTH] = {0};
    unsigned depth = 0;
    unsigned root_fields = 0;
    unsigned metric_fields = 0;
    unsigned sample_fields = 0;
    unsigned documents = 0;
    uint64_t samples = 0;
    uint64_t measured_steps = 0;
    uint64_t failures = 0;
    bool valid = true;
    bool stream_end = false;
    bool samples_complete = false;

    while (valid && !stream_end) {
        yaml_event_t event;
        if (!yaml_parser_parse(&parser, &event)) {
            valid = false;
            break;
        }
        yaml_event_type_t type = event.type;
        if (type == YAML_DOCUMENT_START_EVENT) {
            valid = ++documents == 1;
        } else if (type == YAML_STREAM_END_EVENT) {
            stream_end = true;
            valid = depth == 0;
        } else if (type == YAML_ALIAS_EVENT) {
            valid = false;
        } else if (type == YAML_MAPPING_START_EVENT || type == YAML_SEQUENCE_START_EVENT) {
            valid = depth < REPORT_MAX_DEPTH;
            if (valid && depth == 0) {
                valid = type == YAML_MAPPING_START_EVENT;
            } else if (valid) {
                ReportFrame *parent = &stack[depth - 1];
                if (parent->kind == YAML_MAPPING_START_EVENT) {
                    valid = !parent->expects_key;
                    parent->expects_key = true;
                }
                if (depth == 1 && !strcmp(stack[0].key, "samples")) {
                    valid = valid && type == YAML_SEQUENCE_START_EVENT && !samples_complete;
                }
                if (depth == 2 && !strcmp(stack[0].key, "samples")) {
                    valid = valid && type == YAML_MAPPING_START_EVENT;
                    sample_fields = 0;
                }
                if (depth >= 3 && !strcmp(stack[0].key, "samples")) {
                    valid = false;
                }
            }
            if (valid) {
                stack[depth++] = (ReportFrame){type, true, ""};
            }
        } else if (type == YAML_MAPPING_END_EVENT || type == YAML_SEQUENCE_END_EVENT) {
            valid = depth > 0;
            if (valid) {
                ReportFrame *frame = &stack[depth - 1];
                valid = type == YAML_MAPPING_END_EVENT
                            ? frame->kind == YAML_MAPPING_START_EVENT && frame->expects_key
                            : frame->kind == YAML_SEQUENCE_START_EVENT;
                if (depth == 3 && !strcmp(stack[0].key, "samples")) {
                    valid = valid && sample_fields == 7;
                    samples++;
                }
                if (depth == 2 && !strcmp(stack[0].key, "samples")) {
                    samples_complete = true;
                }
                depth--;
            }
        } else if (type == YAML_SCALAR_EVENT) {
            valid = depth > 0 && event.data.scalar.length < 4096;
            if (valid) {
                ReportFrame *frame = &stack[depth - 1];
                const char *value = (const char *)event.data.scalar.value;
                valid = frame->kind == YAML_MAPPING_START_EVENT;
                if (valid && frame->expects_key) {
                    valid = strlen(value) < sizeof frame->key;
                    snprintf(frame->key, sizeof frame->key, "%s", value);
                    frame->expects_key = false;
                } else if (valid) {
                    const char *key = frame->key;
                    frame->expects_key = true;
                    if (depth == 1) {
                        unsigned bit = 0;
                        if (!strcmp(key, "schema_version")) {
                            bit = 1;
                            valid = !strcmp(value, "1");
                        } else if (!strcmp(key, "project")) {
                            bit = 2;
                            valid = !strcmp(value, "Tether");
                        } else if (!strcmp(key, "kind")) {
                            bit = 4;
                            valid = !strcmp(value, "correctness") || !strcmp(value, "benchmark");
                        } else if (!strcmp(key, "status")) {
                            bit = 8;
                            valid = !strcmp(value, "passed") || !strcmp(value, "failed") ||
                                    !strcmp(value, "cancelled") || !strcmp(value, "rejected");
                        } else if (!strcmp(key, "samples")) {
                            valid = false;
                        }
                        valid = valid && !(root_fields & bit);
                        root_fields |= bit;
                    }
                    if (depth == 2 && !strcmp(stack[0].key, "metrics")) {
                        unsigned bit = 0;
                        uint64_t number = 0;
                        if (!strcmp(key, "measured_steps")) {
                            bit = 1;
                            valid = unsigned_value(value, &measured_steps);
                        } else if (!strcmp(key, "failures")) {
                            bit = 2;
                            valid = unsigned_value(value, &failures);
                        } else if (!strcmp(key, "checks")) {
                            bit = 4;
                            valid = unsigned_value(value, &number);
                        }
                        valid = valid && !(metric_fields & bit);
                        metric_fields |= bit;
                    }
                    if (depth == 3 && !strcmp(stack[0].key, "samples")) {
                        unsigned bit = 0;
                        uint64_t number = 0;
                        if (!strcmp(key, "step")) {
                            bit = 1;
                            valid = unsigned_value(value, &number) && number == samples;
                        } else if (!strcmp(key, "alive")) {
                            bit = 2;
                            valid = unsigned_value(value, &number);
                        } else if (!strcmp(key, "work_ms")) {
                            bit = 4;
                            valid = nonnegative_value(value);
                        } else if (!strcmp(key, "rss_bytes") && strcmp(value, "null")) {
                            uint64_t rss = 0;
                            valid = unsigned_value(value, &rss);
                        }
                        valid = valid && !(sample_fields & bit);
                        sample_fields |= bit;
                    }
                    for (unsigned i = 0; i < REPORT_FIELD_COUNT && valid; i++) {
                        bool location =
                            (i <= REPORT_SUMMARY || i == REPORT_KIND)
                                ? depth == 1
                                : depth == 2 && !strcmp(stack[0].key,
                                                        i >= REPORT_CPU_STATE ? "measurement_state"
                                                        : i <= REPORT_COUNT   ? "parameters"
                                                                              : "metrics");
                        if (location && !strcmp(key, keys[i]) && strcmp(value, "null")) {
                            if (i >= REPORT_COUNT && i < REPORT_CPU_STATE && i != REPORT_KIND) {
                                valid = nonnegative_value(value);
                            } else if (i >= REPORT_CPU_STATE) {
                                valid = !strcmp(value, "measured") ||
                                        !strcmp(value, "query_failed") ||
                                        !strcmp(value, "not_measured");
                            }
                            valid = valid && strlen(value) < sizeof report->value[i];
                            snprintf(report->value[i], sizeof report->value[i], "%s", value);
                        }
                    }
                }
            }
        }
        yaml_event_delete(&event);
    }
    yaml_parser_delete(&parser);
    fclose(file);
    valid = valid && stream_end && documents == 1 && samples_complete && root_fields == 15 &&
            metric_fields == 7 && measured_steps == samples;
    if (report->value[REPORT_CPU_STATE][0]) {
        valid = valid && (!strcmp(report->value[REPORT_CPU_STATE], "measured") ==
                          (report->value[REPORT_CPU][0] != 0));
    }
    if (report->value[REPORT_RSS_STATE][0]) {
        valid = valid && (!strcmp(report->value[REPORT_RSS_STATE], "measured") ==
                          (report->value[REPORT_RSS][0] != 0));
    }
    if (!strcmp(report->value[REPORT_STATUS], "passed")) {
        valid = valid && failures == 0 && samples > 0;
    }
    if (!valid) {
        memset(report, 0, sizeof *report);
    }
    return valid;
}
