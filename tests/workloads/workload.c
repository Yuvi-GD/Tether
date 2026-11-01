#include "tool.h"
#include "tether/tether.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_registry.h"
#include "tether/core/tether_components.h"
#include "tether/ui/tether_widgets.h"
#include "tether/engine/tether_engine.h"
#include "chitra.h"
#include "chitra_table.h"
#include "chitra_rhi.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Tether_GUID id;
    uint64_t value;
} Item;
/* One runner owns one engine on its main thread. Component pointers never
   survive an entity creation/deletion that could relocate dense storage. */
static struct {
    const ToolOptions *o;
    ToolResult *r;
    Item *items;
    uint32_t count, capacity, component, serial, frame, columns;
    Tether_GUID root;
    bool render, finished, measuring;
    double started, cpu_start, frame_start, previous_frame;
    double run_started;
    double warmup_started;
    double shutdown_started;
    Chitra_RHI_API original_rhi, traced_rhi;
    uint32_t draws;
    uint64_t uploads;
} run;

int tool_run_layout(const ToolOptions *options, ToolResult *result);

static void check(bool condition) {
    run.r->checks++;
    if (!condition) {
        run.r->failures++;
    }
}

static uint32_t color(uint64_t value) {
    uint32_t x = (uint32_t)value * 1664525u + run.o->seed * 1013904223u;
    return 0xFF000000u | (x & 0x00FFFFFFu);
}

static void set_color(Tether_GUID id, uint64_t value) {
    uint32_t rgba = color(value);
    Tether_Shape *shape = tether_ecs_get_component(id, TETHER_COMPONENT_SHAPE);
    shape->bg_color =
        (Tether_Color){(uint8_t)rgba, (uint8_t)(rgba >> 8), (uint8_t)(rgba >> 16), 255};
    tether_ecs_add_component(id, TETHER_COMPONENT_DIRTY_VISUAL);
}

static bool append_object(void) {
    if (run.count >= run.capacity) {
        return false;
    }
    uint32_t index = run.count;
    Tether_GUID id = run.render ? tether_widget_create_panel(run.root) : tether_ecs_create_entity();
    if (!tether_ecs_is_valid(id)) {
        return false;
    }
    uint64_t value = ++run.serial;
    run.items[index] = (Item){id, value};
    if (run.render) {
        Tether_AnchorSlot *anchor = tether_ecs_get_component(id, TETHER_COMPONENT_ANCHOR_SLOT);
        float x = (float)(index % run.columns) / run.columns;
        float y = (float)(index / run.columns) / run.columns;
        anchor->anchor_min = (Tether_Vec2){x, y};
        anchor->anchor_max = (Tether_Vec2){x + 0.9f / run.columns, y + 0.9f / run.columns};
        set_color(id, value);
        if (!strcmp(run.o->mode, "volatile")) {
            tether_ecs_add_component(id, TETHER_COMPONENT_VOLATILE);
        }
        tether_widget_show(id);
    } else {
        uint64_t *payload = tether_ecs_add_component(id, run.component);
        if (!payload) {
            tether_ecs_destroy_entity(id);
            return false;
        }
        *payload = value;
    }
    run.count++;
    if (run.count > run.r->peak_active) {
        run.r->peak_active = run.count;
    }
    return true;
}

static void remove_object(void) {
    Tether_GUID id = run.items[--run.count].id;
    if (run.render) {
        tether_widget_destroy(id);
    } else {
        tether_ecs_destroy_entity(id);
    }
    if (run.o->test) {
        check(!tether_ecs_is_valid(id));
    }
}

static void trace_draw(const Chitra_DrawBatch *batch) {
    run.draws++;
    run.original_rhi.draw_batch(batch);
}
static void trace_vertices(uint8_t pipeline, const void *data, size_t size) {
    run.uploads += size;
    run.original_rhi.upload_vertices(pipeline, data, size);
}
static void trace_indices(const uint32_t *data, size_t size) {
    run.uploads += size;
    run.original_rhi.upload_indices(data, size);
}

static void initial_objects(void) {
    double start = tool_now_ms();
    for (uint32_t i = 0; i < run.o->count; i++) {
        if (!append_object()) {
            run.r->failures++;
            break;
        }
    }
    run.r->create_ms = tool_now_ms() - start;
    run.r->initial_active = run.count;
}

static void init_render(void) {
    run.root = tether_widget_create_panel(tether_ecs_get_main_root());
    Tether_Shape *background = tether_ecs_get_component(run.root, TETHER_COMPONENT_SHAPE);
    background->bg_color = (Tether_Color){18, 22, 31, 255};
    tether_widget_show(run.root);
    run.original_rhi = *chitra_rhi_api();
    run.traced_rhi = run.original_rhi;
    run.traced_rhi.draw_batch = trace_draw;
    run.traced_rhi.upload_vertices = trace_vertices;
    run.traced_rhi.upload_indices = trace_indices;
    chitra_rhi_set_api(&run.traced_rhi);
    initial_objects();
    run.r->setup_ms = tool_now_ms() - run.run_started;
    run.warmup_started = tool_now_ms();
}

static void validate_objects(void) {
    double validation_started = tool_now_ms();
    for (uint32_t i = 0; i < run.count; i++) {
        Item item = run.items[i];
        check(tether_ecs_is_valid(item.id));
        if (!run.render) {
            uint64_t *value = tether_ecs_get_component(item.id, run.component);
            check(value && *value == item.value);
        } else {
            Tether_Shape *shape = tether_ecs_get_component(item.id, TETHER_COMPONENT_SHAPE);
            if (!shape) {
                check(false);
                continue;
            }
            uint8_t pipeline = 255;
            uint32_t count = 0;
            Chitra_ShapeVertex *vertices =
                chitra_table_get_vertices(shape->handle, &pipeline, &count);
            if (!vertices || pipeline != CHITRA_PIPELINE_SHAPE || count != 4) {
                check(false);
                continue;
            }
            check(chitra_is_alive(shape->handle));
            Tether_Visibility *visibility =
                tether_ecs_get_component(item.id, TETHER_COMPONENT_VISIBILITY);
            if (visibility->computed_state == TETHER_VISIBLE) {
                int width, height;
                chitra_get_resolution(&width, &height);
                float x = (float)(i % run.columns) / run.columns * width;
                float y = (float)(i / run.columns) / run.columns * height;
                check(fabsf(vertices[0].pos[0] - x) < 0.05f &&
                      fabsf(vertices[0].pos[1] - y) < 0.05f);
                check(vertices[0].fill_color == color(item.value));
            } else {
                check((vertices[0].fill_color >> 24) == 0);
            }
        }
    }
    if (!run.render) {
        Tether_DenseArray *array = tether_ecs_get_dense_array(run.component);
        check(array && array->count == run.count);
    }
    run.r->validation_ms += tool_now_ms() - validation_started;
}

static void begin_measurement(void) {
    if (run.render) {
        run.r->warmup_ms = tool_now_ms() - run.warmup_started;
    }
    run.started = tool_now_ms();
    run.cpu_start = tool_cpu_ms();
    run.r->rss_start = tool_rss();
    run.r->rss_peak = run.r->rss_start;
    run.measuring = true;
}

static void update(void) {
    if (run.finished) {
        return;
    }
    run.frame++;
    run.draws = 0;
    run.uploads = 0;
    if (run.frame <= run.o->warmup) {
        run.r->warmup_frames++;
        if (run.render) {
            chitra_mark_dirty();
        }
        return;
    }
    if (!run.measuring) {
        begin_measurement();
    }
    run.frame_start = tool_now_ms();
    ToolSample *sample = &run.r->samples[run.r->sample_count];
    sample->interval_ms = run.previous_frame ? run.frame_start - run.previous_frame : 0;
    run.previous_frame = run.frame_start;
    double start = tool_now_ms();
    uint32_t removed = run.o->remove < run.count ? run.o->remove : run.count;
    for (uint32_t i = 0; i < removed; i++) {
        remove_object();
    }
    sample->deleted = removed;
    sample->delete_ms = tool_now_ms() - start;
    start = tool_now_ms();
    for (uint32_t i = 0; i < run.o->spawn; i++) {
        if (!append_object()) {
            run.r->failures++;
            break;
        }
        sample->spawned++;
    }
    sample->spawn_ms = tool_now_ms() - start;
    start = tool_now_ms();
    uint32_t mutated = run.o->mutate < run.count ? run.o->mutate : run.count;
    for (uint32_t i = 0; i < mutated; i++) {
        run.items[i].value++;
        if (run.render) {
            set_color(run.items[i].id, run.items[i].value);
        } else {
            uint64_t *payload = tether_ecs_get_component(run.items[i].id, run.component);
            if (payload) {
                *payload = run.items[i].value;
            } else {
                run.r->failures++;
            }
        }
    }
    sample->mutated = mutated;
    sample->mutate_ms = tool_now_ms() - start;
    sample->alive = run.count;
    if (run.render) {
        if (run.o->test && run.count) {
            const Tether_VisibilityState states[] = {TETHER_HIDDEN, TETHER_VISIBLE,
                                                     TETHER_COLLAPSED, TETHER_VISIBLE};
            tether_widget_set_visibility(run.items[0].id, states[run.r->sample_count % 4]);
        }
        /* Require actual rendering even for a static retained scene. */
        chitra_mark_dirty();
    }
}

static void after_frame(void) {
    if (!run.measuring || run.finished) {
        return;
    }
    ToolSample *sample = &run.r->samples[run.r->sample_count];
    sample->work_ms = tool_now_ms() - run.frame_start;
    sample->draws = run.draws;
    sample->upload_bytes = run.uploads;
    run.r->final_active = run.count;
    if (run.render) {
        sample->visible = run.count;
        if (run.count && run.o->test) {
            Tether_Visibility *visibility =
                tether_ecs_get_component(run.items[0].id, TETHER_COMPONENT_VISIBILITY);
            if (visibility && visibility->computed_state != TETHER_VISIBLE) {
                sample->visible--;
            }
        }
    }
    if (run.render) {
        int width, height;
        chitra_get_resolution(&width, &height);
        sample->viewport_width = (uint32_t)width;
        sample->viewport_height = (uint32_t)height;
    }
    if (run.o->test) {
        validate_objects();
    }
    if (run.render) {
        check(run.draws > 0);
    }
    run.r->sample_count++;
    run.r->rss_end = tool_rss();
    sample->rss_bytes = run.r->rss_end;
    if (run.r->rss_end != UINT64_MAX &&
        (run.r->rss_peak == UINT64_MAX || run.r->rss_end > run.r->rss_peak)) {
        run.r->rss_peak = run.r->rss_end;
    }
    run.r->elapsed_ms = tool_now_ms() - run.started;
    run.r->cpu_ms = tool_cpu_ms() - run.cpu_start;
    if (run.r->sample_count >= run.o->frames || run.r->failures ||
        (run.o->seconds > 0 && run.r->elapsed_ms >= run.o->seconds * 1000.0)) {
        if (!run.o->test) {
            validate_objects();
        }
        run.r->termination_reason = run.r->failures                        ? "assertion_failure"
                                    : run.r->sample_count >= run.o->frames ? "step_limit"
                                                                           : "time_limit";
        run.finished = true;
        if (run.render) {
            chitra_quit();
        }
    }
}

static void teardown(void) {
    double start = tool_now_ms();
    run.shutdown_started = start;
    while (run.count) {
        remove_object();
    }
    if (run.render) {
        tether_widget_destroy(run.root);
    }
    run.r->teardown_ms = tool_now_ms() - start;
}

static void hierarchy_test(void) {
    tether_ecs_register_component_static(TETHER_COMPONENT_HIERARCHY, sizeof(Tether_Hierarchy));
    run.r->setup_ms = tool_now_ms() - run.run_started;
    begin_measurement();
    for (uint32_t round = 0; round < run.o->frames; round++) {
        double start = tool_now_ms();
        Tether_GUID parent = tether_ecs_create_entity();
        tether_ecs_add_component(parent, TETHER_COMPONENT_HIERARCHY);
        for (uint32_t i = 0; i < run.o->count; i++) {
            Tether_GUID child = tether_ecs_create_entity();
            run.items[i].id = child;
            tether_ecs_add_component(child, TETHER_COMPONENT_HIERARCHY);
            tether_ecs_attach_entity(parent, child);
        }
        for (uint32_t i = 0; i < run.o->count; i += 2) {
            tether_ecs_detach_entity(run.items[i].id);
        }
        Tether_Hierarchy *h = tether_ecs_get_component(parent, TETHER_COMPONENT_HIERARCHY);
        check(h && h->child_count == run.o->count / 2);
        tether_ecs_destroy_entity(parent);
        check(!tether_ecs_is_valid(parent));
        for (uint32_t i = 0; i < run.o->count; i++) {
            check(tether_ecs_is_valid(run.items[i].id) == (i % 2 == 0));
            if (i % 2 == 0) {
                tether_ecs_destroy_entity(run.items[i].id);
            }
        }
        run.r->samples[round].work_ms = tool_now_ms() - start;
        run.r->peak_active = run.o->count + 1;
        run.r->sample_count++;
        run.r->elapsed_ms = tool_now_ms() - run.started;
        run.r->cpu_ms = tool_cpu_ms() - run.cpu_start;
        run.r->rss_end = tool_rss();
        run.r->samples[round].rss_bytes = run.r->rss_end;
        if (run.r->rss_end != UINT64_MAX &&
            (run.r->rss_peak == UINT64_MAX || run.r->rss_end > run.r->rss_peak)) {
            run.r->rss_peak = run.r->rss_end;
        }
        if (run.r->failures || (run.o->seconds > 0 && run.r->elapsed_ms >= run.o->seconds * 1000)) {
            break;
        }
    }
    run.r->termination_reason = run.r->failures                        ? "assertion_failure"
                                : run.r->sample_count >= run.o->frames ? "step_limit"
                                                                       : "time_limit";
    run.finished = true;
}

int tool_run(const ToolOptions *o, ToolResult *r) {
    memset(&run, 0, sizeof run);
    run.o = o;
    run.r = r;
    r->cpu_ms = NAN;
    r->rss_start = UINT64_MAX;
    r->rss_end = UINT64_MAX;
    r->rss_peak = UINT64_MAX;
    run.run_started = tool_now_ms();
    r->termination_reason = "window_closed";
    if (!tool_preflight(o, &run.capacity, r->message, sizeof r->message)) {
        r->status = "rejected";
        r->termination_reason = "preflight_rejected";
        r->total_run_ms = tool_now_ms() - run.run_started;
        return 2;
    }
    if (!strcmp(o->scenario, "layout")) {
        return tool_run_layout(o, r);
    }
    run.items = calloc(run.capacity, sizeof(Item));
    r->samples = calloc(o->frames, sizeof(ToolSample));
    if (!run.items || !r->samples) {
        free(run.items);
        r->termination_reason = "allocation_failure";
        r->total_run_ms = tool_now_ms() - run.run_started;
        snprintf(r->message, sizeof r->message, "Runner allocation failed.");
        return 2;
    }
    run.columns = (uint32_t)ceil(sqrt((double)run.capacity));
    run.render = !strcmp(o->scenario, "render") || !strcmp(o->scenario, "ui");
    if (run.render) {
#if defined(SOKOL_D3D11)
        r->backend = "Chitra/Sokol D3D11";
#elif defined(SOKOL_METAL)
        r->backend = "Chitra/Sokol Metal";
#elif defined(SOKOL_GLCORE)
        r->backend = "Chitra/Sokol GLCore";
#else
        r->backend = "Chitra/Sokol";
#endif
        Tether_App_Config config = {0};
        config.width = 1280;
        config.height = 720;
        config.title = o->test ? "Tether UI correctness" : "Tether rendered workload";
        config.target_fps = (int)o->fps;
        config.on_init = init_render;
        config.on_frame = update;
        config.on_post_frame = after_frame;
        config.on_term = teardown;
        tether_run(&config);
        chitra_rhi_set_api(NULL);
    } else {
        tether_ecs_init();
        tether_registry_init();
        run.component = tether_ecs_register_component_dynamic(sizeof(uint64_t));
        if (!strcmp(o->scenario, "hierarchy")) {
            hierarchy_test();
        } else {
            initial_objects();
            r->setup_ms = tool_now_ms() - run.run_started;
            /* Warmup only applies to rendered scenarios. */
            run.frame = o->warmup;
            while (!run.finished) {
                update();
                after_frame();
            }
            teardown();
        }
        if (!run.shutdown_started) {
            run.shutdown_started = tool_now_ms();
        }
        tether_registry_term();
        tether_ecs_term();
    }
    if (run.shutdown_started) {
        r->shutdown_ms = tool_now_ms() - run.shutdown_started;
    }
    r->total_run_ms = tool_now_ms() - run.run_started;
    r->status = r->failures ? "failed" : run.finished ? "passed" : "cancelled";
    uint64_t draws = 0;
    for (uint32_t i = 0; i < r->sample_count; i++) {
        draws += r->samples[i].draws;
    }
    snprintf(r->message, sizeof r->message,
             "%s: %s; %u steps, %llu checks, %llu failures, %.2f ms measured wall time, %llu draw "
             "submissions.",
             o->scenario, r->status, r->sample_count, (unsigned long long)r->checks,
             (unsigned long long)r->failures, r->elapsed_ms, (unsigned long long)draws);
    free(run.items);
    return r->failures ? 1 : run.finished ? 0 : 3;
}
