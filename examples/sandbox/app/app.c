#include "app.h"
#include "sandbox_ui.h"
#include "lab.h"
#include "results.h"
#include "tool.h"
#include "sandbox_widget.h"
#include "sandbox_events.h"
#include "tether/tether.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/engine/parser/tether_yaml.h"
#include "tether/core/tether_registry.h"
#include "chitra.h"
#include <stdlib.h>
#include <string.h>

static Tether_GUID views[3];
static SandboxView wanted = SANDBOX_LAB;
static int current = -1, exit_code;
static bool showcase_loaded, smoke, smoke_detach, smoke_invalid;
static bool smoke_report;
static char smoke_expected_badge[32];
static double deadline;
static char initial_result[4096];

void sandbox_navigate(SandboxView view) {
    wanted = view;
}

Tether_GUID sandbox_showcase_parent(void) {
    return views[SANDBOX_SHOWCASE];
}

void sandbox_mount_scene(Tether_GUID root) {
    tether_ui_set_flex(root, 1.0f);
}

static void navigate(Tether_GUID id, Tether_EventType event, void *data) {
    (void)id;
    (void)event;
    sandbox_navigate((SandboxView)(uintptr_t)data);
}

static void switch_view(void) {
    if (current == (int)wanted) {
        return;
    }
    if (wanted == SANDBOX_SHOWCASE && !showcase_loaded) {
        Tether_GUID root = tether_yaml_load("UI/index.yaml", views[SANDBOX_SHOWCASE]);
        if (root == TETHER_INVALID_GUID) {
            results_message("Cannot load showcase UI/index.yaml.");
            wanted = SANDBOX_RESULTS;
        } else {
            sandbox_mount_scene(root);
            sandbox_bind_events();
            showcase_loaded = true;
        }
    }
    bool show_workbench = (wanted == SANDBOX_LAB || wanted == SANDBOX_RESULTS);
    tether_widget_set_visibility(views[SANDBOX_LAB],
                                 show_workbench ? TETHER_VISIBLE : TETHER_COLLAPSED);
    tether_widget_set_visibility(views[SANDBOX_SHOWCASE],
                                 wanted == SANDBOX_SHOWCASE ? TETHER_VISIBLE : TETHER_COLLAPSED);
    current = (int)wanted;
}

static void init(void) {
    tether_font_register("Roboto-Regular", "Font/Roboto-Regular.ttf");
    tether_register_widget("MyCModal", create_my_c_modal);
    lab_register();
    results_register();
    Tether_GUID app_root = tether_ui_column(tether_ecs_get_main_root(), 0.0f);

    Tether_GUID nav = tether_ui_row(app_root, 8.0f);
    tether_ui_set_size(nav, 0.0f, 72.0f);
    tether_ui_set_padding(nav, 12.0f, 12.0f, 12.0f, 12.0f);
    tether_ui_set_color(nav, (Tether_Color){24, 30, 42, 255});

    sandbox_ui_button(nav, "Lab & Results", navigate, (void *)(uintptr_t)SANDBOX_LAB);
    sandbox_ui_button(nav, "Showcase", navigate, (void *)(uintptr_t)SANDBOX_SHOWCASE);

    for (int i = 0; i < 2; i++) {
        views[i] = tether_ui_panel(app_root, (Tether_Color){0, 0, 0, 0}, 0.0f);
        tether_ui_set_flex(views[i], 1.0f);
    }
    views[SANDBOX_RESULTS] = views[SANDBOX_LAB];
    Tether_GUID lab = tether_yaml_load("UI/lab.yaml", views[SANDBOX_LAB]);
    if (lab == TETHER_INVALID_GUID) {
        exit_code = 1;
        chitra_quit();
        return;
    }
    sandbox_mount_scene(lab);
    results_create(views[SANDBOX_RESULTS]);
    tether_widget_show(app_root);
    if (initial_result[0]) {
        results_load(initial_result);
        wanted = SANDBOX_RESULTS;
    }
    switch_view();
    if (smoke) {
        deadline = tool_now_ms() + 15000;
        lab_smoke_launch();
    }
    if (smoke_detach) {
        lab_smoke_detach();
    }
}

static void update(void) {
    lab_update();
    switch_view();
    if (smoke && tool_now_ms() > deadline) {
        exit_code = 1;
        chitra_quit();
    }
}

static void after_frame(void) {
    static unsigned smoke_stage;
    if (smoke_report) {
        const char *status = tether_ui_get_text(tether_registry_find_entity("res_status_badge"));
        exit_code = status && !strcmp(status, smoke_expected_badge) && results_has_geometry()
                        ? 0 : 1;
        printf("Sandbox report-status display: %s\n", exit_code ? "FAILED" : "PASSED");
        smoke_report = false;
        chitra_quit();
        return;
    }
    if (smoke_invalid) {
        const char *message = tether_ui_get_text(tether_registry_find_entity("res_report_text"));
        const char *status = tether_ui_get_text(tether_registry_find_entity("res_status_badge"));
        exit_code = message && strstr(message, "incomplete or unsupported report") && status &&
                            !strcmp(status, "NOTICE") && results_has_geometry()
                        ? 0 : 1;
        printf("Sandbox invalid-report display: %s\n", exit_code ? "FAILED" : "PASSED");
        smoke_invalid = false;
        chitra_quit();
        return;
    }
    if (smoke && smoke_stage == 1 && current == SANDBOX_SHOWCASE) {
        Tether_GUID root = tether_registry_find_entity("AppRoot");
        Tether_Hierarchy *hierarchy = tether_ecs_get_component(root, TETHER_COMPONENT_HIERARCHY);
        if (!hierarchy || hierarchy->parent != views[SANDBOX_SHOWCASE]) {
            exit_code = 1;
        }
        smoke_stage = 2;
        sandbox_navigate(SANDBOX_RESULTS);
        return;
    }
    if (smoke && lab_smoke_status() >= 0 && current == SANDBOX_RESULTS) {
        if (lab_smoke_status()) {
            exit_code = lab_smoke_status();
        }
        if (!results_has_geometry() || !lab_has_geometry()) {
            exit_code = 1;
        }
        if (!exit_code && smoke_stage == 0) {
            smoke_stage = 1;
            sandbox_navigate(SANDBOX_SHOWCASE);
            return;
        }
        printf("Sandbox launch/import/render smoke: %s\n", exit_code ? "FAILED" : "PASSED");
        smoke = false;
        chitra_quit();
    }
}

int sandbox_run(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--smoke")) {
            smoke = true;
        } else if (!strcmp(argv[i], "--smoke-detach")) {
            smoke_detach = true;
        } else if (!strcmp(argv[i], "--smoke-invalid-report") && i + 1 < argc) {
            smoke_invalid = true;
            if (snprintf(initial_result, sizeof initial_result, "%s", argv[++i]) >=
                (int)sizeof initial_result) {
                return 2;
            }
        } else if (!strcmp(argv[i], "--smoke-report") && i + 2 < argc) {
            smoke_report = true;
            if (snprintf(initial_result, sizeof initial_result, "%s", argv[++i]) >=
                    (int)sizeof initial_result ||
                snprintf(smoke_expected_badge, sizeof smoke_expected_badge, "%s", argv[++i]) >=
                    (int)sizeof smoke_expected_badge) {
                return 2;
            }
        } else if (!strcmp(argv[i], "--showcase")) {
            wanted = SANDBOX_SHOWCASE;
        } else if (!strcmp(argv[i], "--result") && i + 1 < argc) {
#ifdef _WIN32
            if (!_fullpath(initial_result, argv[++i], sizeof initial_result)) {
                return 2;
            }
#else
            if (!realpath(argv[++i], initial_result)) {
                return 2;
            }
#endif
        } else {
            printf(
                "Usage: tether_sandbox [--showcase | --result PATH | --smoke | "
                "--smoke-detach | --smoke-invalid-report PATH | "
                "--smoke-report PATH BADGE]\n");
            return 2;
        }
    }
    if (!tool_enter_executable_directory()) {
        return 2;
    }
    Tether_App_Config config = {0};
    config.width = 1200;
    config.height = 830;
    config.title = "Tether Sandbox";
    config.target_fps = 60;
    config.on_init = init;
    config.on_frame = update;
    config.on_post_frame = after_frame;
    config.on_term = lab_cleanup;
    tether_run(&config);
    return exit_code;
}
