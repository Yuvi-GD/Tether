#include "app.h"
#include "lab.h"
#include "results.h"
#include "sandbox_ui.h"
#include "tool.h"
#include "tether/tether.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_registry.h"
#include "tether/core/tether_events.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/engine/parser/tether_yaml.h"
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
static const char *names[] = {"Scenario", "Objects", "Spawn",  "Delete",     "Mutate", "Step limit",
                              "Seconds",  "Mode",    "Warmup", "Memory MiB", "FPS cap"};
static const char *scenarios[] = {"Render benchmark", "ECS benchmark", "Lifecycle test",
                                  "Hierarchy test", "Rendered UI test"};
static const char *scenario_args[] = {"render", "ecs", "lifecycle", "hierarchy", "ui"};
static const uint32_t counts[] = {100, 1000, 10000, 100000, 1000000, 10000000};
static const uint32_t batches[] = {0, 1, 10, 100, 1000};
static const uint32_t frames[] = {4, 120, 300, 600};
static const uint32_t seconds[] = {0, 5, 10, 30};
static const uint32_t budgets[] = {1024, 4096, 16384, 32768};
static const uint32_t warmups[] = {0, 30, 60};
static const uint32_t fps_values[] = {0, 60, 120, 1000};
static unsigned selected[FIELD_COUNT] = {0, 1, 0, 0, 2, 1, 0, 0, 1, 0, 1};
static Tether_GUID labels[FIELD_COUNT], status_label;
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
        return seconds[i];
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
static void refresh_field(unsigned field) {
    char value[100];
    if (!field_applies(field)) {
        snprintf(value, sizeof value, "%s: N/A", names[field]);
    } else if (field == SCENARIO) {
        snprintf(value, sizeof value, "%s", scenarios[selected[field]]);
    } else if (field == MODE) {
        snprintf(value, sizeof value, "%s", selected[field] ? "Volatile" : "Retained");
    } else {
        snprintf(value, sizeof value, "%s: %u", names[field], numeric(field));
    }
    tether_ui_set_text(labels[field], value);
    tether_ui_set_text_color(labels[field], field_applies(field)
                                                ? (Tether_Color){222, 232, 247, 255}
                                                : (Tether_Color){125, 140, 158, 255});
}
static void change_field(Tether_GUID entity, Tether_EventType event, void *data) {
    (void)entity;
    (void)event;
    unsigned field = (unsigned)(uintptr_t)data;
    if (process.active || !field_applies(field)) {
        return;
    }
    const unsigned lengths[] = {5, 6, 5, 5, 5, 4, 4, 2, 3, 4, 4};
    selected[field] = (selected[field] + 1) % lengths[field];
    for (unsigned i = 0; i < FIELD_COUNT; i++) {
        refresh_field(i);
    }
    if (field == SCENARIO) {
        tether_ui_set_text(
            status_label,
            selected[field] == 3
                ? "Hierarchy: repeat create / detach / destroy. Per-step edits and rendering "
                  "controls do not apply."
                : "Click to cycle presets; exact values are accepted by the CLI. Tests check "
                  "behavior; benchmarks measure work. Other regression checks run through CTest.");
    }
}

static void launch_workload(Tether_GUID entity, Tether_EventType event, void *data) {
    (void)entity;
    (void)event;
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
        if (data) {
            chitra_quit();
        }
    }
}

static void field_row(Tether_GUID parent, const unsigned *fields, unsigned count) {
    Tether_GUID row = tether_ui_row(parent, 10.0f);
    tether_ui_set_size(row, 0.0f, 48.0f);
    for (unsigned j = 0; j < count; j++) {
        unsigned i = fields[j];
        labels[i] = sandbox_ui_button(row, "", change_field, (void *)(uintptr_t)i);
        refresh_field(i);
    }
}

static Tether_GUID controls(Tether_GUID parent) {
    Tether_GUID root = tether_ui_column(parent, 12.0f);
    const unsigned workload[] = {SCENARIO}, population[] = {COUNT, BUDGET};
    const unsigned edits[] = {SPAWN, DELETE_COUNT, MUTATE}, limits[] = {FRAMES, SECONDS};
    const unsigned rendering[] = {MODE, WARMUP, FPS};
    field_row(sandbox_ui_section(root, "1 / SELECT WORKLOAD"), workload, 1);
    Tether_GUID group = sandbox_ui_section(root, "2 / POPULATION & CHANGES PER STEP");
    field_row(group, population, 2);
    field_row(group, edits, 3);
    field_row(sandbox_ui_section(root, "3 / RUN LIMITS (first limit ends the run)"), limits, 2);
    field_row(sandbox_ui_section(root, "4 / RENDERING"), rendering, 3);
    sandbox_ui_line(root, 1.0f, (Tether_Color){145, 190, 225, 255});
    Tether_GUID actions = tether_ui_row(root, 10.0f);
    tether_ui_set_text_color(actions, (Tether_Color){167, 210, 185, 255});
    tether_ui_set_size(actions, 0.0f, 48.0f);
    sandbox_ui_button(actions, "Run workload", launch_workload, NULL);
    sandbox_ui_button(actions, "Run & close sandbox", launch_workload, (void *)1);
    status_label = tether_ui_text(
        root,
        "Click to cycle presets; exact values are accepted by the CLI. Tests check behavior; "
        "benchmarks measure work. Other regression checks run through CTest.",
        16.0f, (Tether_Color){167, 210, 185, 255});
    Tether_Text *style = tether_ecs_get_component(status_label, TETHER_COMPONENT_TEXT);
    if (style) {
        style->wrap_width = 460;
    }
    return root;
}

void lab_register(void) {
    tether_register_widget("LabControls", controls);
}

void lab_smoke_launch(void) {
    smoke = true;
    selected[SCENARIO] = 2;
    selected[COUNT] = 0;
    selected[FRAMES] = 0;
    launch_workload(0, TETHER_EVENT_CLICK, NULL);
}

void lab_smoke_detach(void) {
    selected[SCENARIO] = 0;
    selected[COUNT] = 0;
    selected[FRAMES] = 2;
    launch_workload(0, TETHER_EVENT_CLICK, (void *)1);
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
        Tether_GUID id = i < FIELD_COUNT ? labels[i] : status_label;
        Tether_Text *text = tether_ecs_get_component(id, TETHER_COMPONENT_TEXT);
        Tether_SlotTransform *position =
            tether_ecs_get_component(id, TETHER_COMPONENT_SLOT_TRANSFORM);
        uint8_t pipeline;
        uint32_t count = 0;
        if (!text || !position || !chitra_table_get_vertices(text->handle, &pipeline, &count) ||
            !count || text->intrinsic_size.x > position->width + 1 || position->y < view->y ||
            position->y + text->intrinsic_size.y > view->y + view->height) {
            return false;
        }
    }
    return true;
}
