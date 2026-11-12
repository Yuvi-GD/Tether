#include "app.h"
#include "lab.h"
#include "results.h"
#include "tool.h"
#include "tether/tether.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_registry.h"
#include "tether/core/tether_events.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "chitra.h"
#include "chitra_table.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
    SCENARIO,
    COUNT,
    SPAWN,
    DELETE_COUNT,
    MUTATE,
    FRAMES,
    SECONDS,
    MODE,
    WARMUP,
    BUDGET,
    FPS,
    FIELD_COUNT
};
static const char *names[] = {"Workload", "Objects", "Spawn / step", "Delete / step", "Mutate / step",
                              "Step limit", "Duration", "Render mode", "Warm-up", "Memory (MiB)", "Frame cap"};
static const char *scenario_args[] = {"render", "ecs", "lifecycle", "hierarchy", "ui"};
static const uint32_t counts[] = {100, 1000, 10000, 100000, 1000000, 10000000};
static const uint32_t batches[] = {0, 1, 10, 100, 1000};
static const uint32_t frames[] = {4, 120, 300, 600};
static const uint32_t seconds[] = {0, 5, 10, 30};
static const uint32_t budgets[] = {1024, 4096, 16384, 32768};
static const uint32_t warmups[] = {0, 30, 60};
static const uint32_t fps_values[] = {0, 60, 120, 1000};
static unsigned selected[FIELD_COUNT];
static Tether_GUID controls[FIELD_COUNT], labels[FIELD_COUNT], status_label;
static Tether_GUID time_toggle, close_checkbox;
static bool refreshing, time_enabled;
static const char *control_ids[] = {
    "lab_scenario", "lab_count", "lab_spawn", "lab_delete", "lab_mutate", "lab_frames",
    "lab_seconds", "lab_mode", "lab_warmup", "lab_budget", "lab_fps"
};
static const unsigned lengths[] = {5, 6, 5, 5, 5, 4, 4, 2, 3, 4, 4};
static ToolProcess process;
static char result_path[256];
static unsigned run_number;
static bool smoke, smoke_done;
static int smoke_exit;

static uint32_t numeric(unsigned field) {
    unsigned i = selected[field];
    switch (field) {
    case COUNT:
        return counts[i];
    case SPAWN:
    case DELETE_COUNT:
    case MUTATE:
        return batches[i];
    case FRAMES:
        return frames[i];
    case SECONDS:
        return time_enabled ? seconds[i] : 0;
    case WARMUP:
        return warmups[i];
    case BUDGET:
        return budgets[i];
    case FPS:
        return fps_values[i];
    default:
        return 0;
    }
}

static bool field_applies(unsigned field) {
    bool rendered = selected[SCENARIO] == 0 || selected[SCENARIO] == 4;
    if (!rendered && (field == MODE || field == WARMUP || field == FPS)) {
        return false;
    }
    if (selected[SCENARIO] == 3 && (field == SPAWN || field == DELETE_COUNT || field == MUTATE)) {
        return false;
    }
    return true;
}
static void refresh_fields(void) {
    refreshing = true;
    for (unsigned field = 0; field < FIELD_COUNT; ++field) {
        Tether_GUID control = controls[field];
        if (field == MODE || field == WARMUP) {
            tether_ui_set_segment_selected(control, (int)selected[field]);
        } else if (field == BUDGET) {
            tether_ui_set_radio_selected(control, (int)selected[field]);
        } else if (field == SECONDS) {
            tether_ui_set_slider_value(control, (float)selected[field]);
        } else {
            tether_ui_set_dropdown_selected(control, (int)selected[field]);
            tether_ui_set_dropdown_open(control, false);
        }
        bool applicable = field_applies(field) && (field != SECONDS || time_enabled);
        tether_widget_set_enabled(control, applicable && !process.active);
        tether_ui_set_text_color(labels[field], applicable
            ? (Tether_Color){195, 195, 195, 255} : (Tether_Color){120, 120, 120, 255});
        char caption[80];
        if (field == SECONDS) {
            snprintf(caption, sizeof caption, "Duration: %u seconds", numeric(SECONDS));
        } else {
            snprintf(caption, sizeof caption, "%s%s", names[field], applicable ? "" : " (not used)");
        }
        tether_ui_set_text(labels[field], caption);
    }
    tether_widget_set_enabled(time_toggle, !process.active);
    tether_widget_set_enabled(close_checkbox, !process.active);
    refreshing = false;
}

static int read_field(unsigned field) {
    Tether_GUID entity = controls[field];
    if (field == MODE || field == WARMUP) return tether_ui_get_segment_selected(entity);
    if (field == BUDGET) return tether_ui_get_radio_selected(entity);
    if (field == SECONDS) return (int)tether_ui_get_slider_value(entity);
    return tether_ui_get_dropdown_selected(entity);
}

static void change_field(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)event;
    if (refreshing) return;
    unsigned field = (unsigned)(uintptr_t)data;
    (void)entity;
    int index = read_field(field);
    /* Dropdown also emits CHANGE on open/close. Do not close it for those events. */
    if (index == (int)selected[field]) return;
    if (!process.active && field_applies(field) && (field != SECONDS || time_enabled) &&
        index >= 0 && (unsigned)index < lengths[field]) {
        selected[field] = (unsigned)index;
        if (field == SCENARIO) {
            refresh_fields();
        } else if (field == SECONDS) {
            char caption[64];
            snprintf(caption, sizeof caption, "Duration: %u seconds", numeric(SECONDS));
            tether_ui_set_text(labels[field], caption);
        }
        return;
    }
    /* Restore controls changed while their setting is unavailable or a run is active. */
    refresh_fields();
}

static void change_time_limit(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)event;
    (void)data;
    if (refreshing || process.active) return;
    time_enabled = tether_ui_get_toggle(entity);
    if (time_enabled && selected[SECONDS] == 0) selected[SECONDS] = 1;
    refresh_fields();
}

static void launch_workload(Tether_GUID entity, const Tether_Event *event_info, void *data) {
    (void)event_info;
    (void)entity;
    (void)data;
    if (process.active) {
        return;
    }
#if !SANDBOX_HAS_RUNNERS
    tether_ui_set_text(status_label, "Build with TETHER_BUILD_RUNNERS=ON to launch workloads.");
    return;
#endif
    if (!tool_make_results_directory("report") ||
        !tool_make_results_directory(SANDBOX_REPORT_DIR)) {
        tether_ui_set_text(status_label, "Cannot create report/workbench beside the app.");
        return;
    }
    unsigned scenario = selected[SCENARIO];
    char count[16], spawn[16], remove[16], mutate[16], limit[16], duration[16], warmup[16],
        budget[16], fps[16];
    snprintf(count, sizeof count, "%u", numeric(COUNT));
    snprintf(spawn, sizeof spawn, "%u", numeric(SPAWN));
    snprintf(remove, sizeof remove, "%u", numeric(DELETE_COUNT));
    snprintf(mutate, sizeof mutate, "%u", numeric(MUTATE));
    snprintf(limit, sizeof limit, "%u", numeric(FRAMES));
    snprintf(duration, sizeof duration, "%u", numeric(SECONDS));
    snprintf(warmup, sizeof warmup, "%u", numeric(WARMUP));
    snprintf(budget, sizeof budget, "%u", numeric(BUDGET));
    snprintf(fps, sizeof fps, "%u", numeric(FPS));
    if (scenario == 3) {
        strcpy(spawn, "0");
        strcpy(remove, "0");
        strcpy(mutate, "0");
    }
    int written =
        snprintf(result_path, sizeof result_path, "%s/%lld-%u-%u.json", SANDBOX_REPORT_DIR,
                 (long long)time(NULL), tool_process_id(), ++run_number);
    if (written < 0 || (size_t)written >= sizeof result_path) {
        tether_ui_set_text(status_label, "Report path is too long.");
        return;
    }
    const char *args[32] = {"--scenario",
                            scenario_args[scenario],
                            "--count",
                            count,
                            "--spawn",
                            spawn,
                            "--delete",
                            remove,
                            "--mutate",
                            mutate,
                            "--frames",
                            limit,
                            "--mode",
                            selected[MODE] ? "volatile" : "retained",
                            "--warmup",
                            warmup,
                            "--memory-mib",
                            budget,
                            "--fps",
                            fps,
                            "--output",
                            result_path,
                            NULL};
    if (numeric(SECONDS)) {
        args[22] = "--seconds";
        args[23] = duration;
        args[24] = NULL;
    }
    if (!tool_launch(&process, scenario < 2 ? "tether_bench" : "tether_test", args)) {
        tether_ui_set_text(
            status_label,
            "Launch failed. Build tether_bench and tether_test beside this application.");
        smoke_exit = 1;
        smoke_done = smoke;
    } else {
        tether_ui_set_text(status_label, "Running independently. Results appear after exit. "
                                         "Closing this app leaves the workload running.");
        refresh_fields();
        if (data || tether_ui_get_checked(close_checkbox)) {
            chitra_quit();
        }
    }
}

bool lab_bind(void) {
    for (unsigned i = 0; i < FIELD_COUNT; ++i) {
        controls[i] = tether_registry_find_entity(control_ids[i]);
        char label_id[64];
        snprintf(label_id, sizeof label_id, "label_%s", control_ids[i]);
        labels[i] = tether_registry_find_entity(label_id);
        if (!controls[i] || !labels[i]) return false;
        int index = read_field(i);
        if (index < 0 || (unsigned)index >= lengths[i]) return false;
        selected[i] = (unsigned)index;
        tether_bind_event(controls[i], TETHER_EVENT_CHANGE, change_field, (void *)(uintptr_t)i);
    }
    status_label = tether_registry_find_entity("lab_status");
    time_toggle = tether_registry_find_entity("lab_time_enabled");
    close_checkbox = tether_registry_find_entity("lab_close");
    Tether_GUID run = tether_registry_find_entity("lab_run");
    if (!status_label || !time_toggle || !close_checkbox || !run) return false;
    time_enabled = tether_ui_get_toggle(time_toggle);
    tether_bind_event(time_toggle, TETHER_EVENT_CHANGE, change_time_limit, NULL);
    tether_bind_event(run, TETHER_EVENT_CLICK, launch_workload, NULL);
    refresh_fields();
    return true;
}

void lab_smoke_launch(void) {
    smoke = true;
    /* Exercise actual widget CHANGE bindings, not just the backing C array. */
    tether_ui_set_dropdown_selected(controls[COUNT], 5);
    tether_ui_set_radio_selected(controls[BUDGET], 1);
    tether_ui_set_segment_selected(controls[MODE], 1);
    tether_ui_set_toggle(time_toggle, true);
    tether_ui_set_slider_value(controls[SECONDS], 2);
    bool valid = numeric(COUNT) == 10000000 && numeric(BUDGET) == 4096 &&
                 selected[MODE] == 1 && numeric(SECONDS) == 10;
    tether_ui_set_toggle(time_toggle, false);
    tether_ui_set_segment_selected(controls[MODE], 0);
    tether_ui_set_radio_selected(controls[BUDGET], 0);
    tether_ui_set_dropdown_selected(controls[SCENARIO], 2);
    tether_ui_set_dropdown_selected(controls[COUNT], 0);
    tether_ui_set_dropdown_selected(controls[FRAMES], 0);
    tether_ui_set_checked(close_checkbox, true);
    valid = valid && tether_ui_get_checked(close_checkbox);
    tether_ui_set_checked(close_checkbox, false);
    valid = valid && numeric(SECONDS) == 0 && selected[SCENARIO] == 2 &&
            numeric(COUNT) == 100 && numeric(FRAMES) == 4;
    printf("Sandbox native-control bindings: %s\n", valid ? "PASSED" : "FAILED");
    if (!valid) {
        smoke_exit = 1;
        smoke_done = true;
        sandbox_navigate(SANDBOX_RESULTS);
        return;
    }
    launch_workload(0, &(Tether_Event){ .type = TETHER_EVENT_CLICK }, NULL);
}

void lab_smoke_detach(void) {
    selected[SCENARIO] = 0;
    selected[COUNT] = 0;
    selected[FRAMES] = 2;
    launch_workload(0, &(Tether_Event){ .type = TETHER_EVENT_CLICK }, (void *)1);
    printf("Detached result: %s\n", result_path);
}

int lab_smoke_status(void) {
    return smoke_done ? smoke_exit : -1;
}

void lab_update(void) {
    static double next_poll;
    double now = tool_now_ms();
    if (now < next_poll) {
        return;
    }
    next_poll = now + 250;
    int code;
    if (tool_poll(&process, &code)) {
        bool loaded = results_load_after_process(result_path, code);
        if (!loaded && !code) {
            code = 1;
        }
        refresh_fields();
        sandbox_navigate(SANDBOX_RESULTS);
        tether_ui_set_text(status_label, "Finished. The cards show the completed run. Change "
                                         "parameters to run again, or load a saved report.");
        if (smoke) {
            smoke_exit = code;
            smoke_done = true;
        }
    }
}

void lab_cleanup(void) {
    tool_detach(&process);
}

bool lab_has_geometry(void) {
    Tether_SlotTransform *view = tether_ecs_get_component(tether_registry_find_entity("LabRoot"),
                                                          TETHER_COMPONENT_SLOT_TRANSFORM);
    if (!view) {
        return false;
    }
    for (unsigned i = 0; i <= FIELD_COUNT; i++) {
        if (i < FIELD_COUNT) {
            Tether_SlotTransform *control = tether_ecs_get_component(controls[i], TETHER_COMPONENT_SLOT_TRANSFORM);
            if (!control || control->width <= 0 || control->height <= 0 ||
                control->y < view->y || control->y + control->height > view->y + view->height + 1) {
                fprintf(stderr, "Lab control geometry failed: %s\n", control_ids[i]);
                return false;
            }
        }
        Tether_GUID id = i < FIELD_COUNT ? labels[i] : status_label;
        Tether_Text *text = tether_ecs_get_component(id, TETHER_COMPONENT_TEXT);
        Tether_SlotTransform *position =
            tether_ecs_get_component(id, TETHER_COMPONENT_SLOT_TRANSFORM);
        uint8_t pipeline;
        uint32_t count = 0;
        if (!text || !position || !chitra_table_get_vertices(text->handle, &pipeline, &count) ||
            !count || text->intrinsic_size.x > position->width + 1 || position->y < view->y ||
            position->y + text->intrinsic_size.y > view->y + view->height) {
            fprintf(stderr, "Lab label geometry failed: %s\n", i < FIELD_COUNT ? control_ids[i] : "status");
            return false;
        }
    }
    return true;
}
