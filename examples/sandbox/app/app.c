#include "app.h"
#include "lab.h"
#include "results.h"
#include "tool.h"
#include "sandbox_events.h"
#include "sandbox_widgets_zoo.h"
#include "tether/tether.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/engine/parser/tether_yaml.h"
#include "tether/core/tether_registry.h"
#include "chitra.h"
#include <stdlib.h>
#include <string.h>

static Tether_GUID views[4];
static SandboxView wanted = SANDBOX_WIDGETS;
static int current = -1, exit_code;
static bool syncing_navigation;
static bool showcase_loaded, smoke, smoke_detach, smoke_invalid;
static bool refresh_requested;
static bool smoke_report;
static char smoke_expected_badge[32];
static double deadline;
static char initial_result[4096];

static void sandbox_refresh(void) {
    refresh_requested = true;
}

static void on_refresh_clicked(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity;
    (void)event;
    (void)data;
    sandbox_refresh();
}

void sandbox_navigate(SandboxView view) {
    wanted = view;
}

static void sandbox_mount_scene(Tether_GUID root) {
    tether_ui_set_flex(root, 1.0f);
}

static void navigate(Tether_GUID id, const Tether_Event *event_info, void *data) {
    (void)event_info;
    (void)data;
    if (syncing_navigation) return;
    int index = tether_ui_get_segment_selected(id);
    if (index >= 0 && index < 3) sandbox_navigate((SandboxView)index);
}

static void switch_view(void) {
    if (current == (int)wanted) {
        return;
    }
    if (current == SANDBOX_SHOWCASE && wanted != SANDBOX_SHOWCASE) {
        sandbox_showcase_close_overlays();
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
    tether_widget_set_visibility(views[SANDBOX_WIDGETS],
                                 wanted == SANDBOX_WIDGETS ? TETHER_VISIBLE : TETHER_COLLAPSED);
    tether_widget_set_visibility(views[SANDBOX_LAB],
                                 show_workbench ? TETHER_VISIBLE : TETHER_COLLAPSED);
    tether_widget_set_visibility(views[SANDBOX_SHOWCASE],
                                 wanted == SANDBOX_SHOWCASE ? TETHER_VISIBLE : TETHER_COLLAPSED);
    current = (int)wanted;
    syncing_navigation = true;
    tether_ui_set_segment_selected(tether_registry_find_entity("app_navigation"),
                                  wanted == SANDBOX_RESULTS ? SANDBOX_LAB : (int)wanted);
    syncing_navigation = false;
}

static bool create_views(void) {
    Tether_GUID app_root = tether_yaml_load("UI/app.yaml", tether_ecs_get_main_root());
    views[SANDBOX_WIDGETS] = tether_registry_find_entity("view_widgets");
    views[SANDBOX_LAB] = tether_registry_find_entity("view_lab");
    views[SANDBOX_SHOWCASE] = tether_registry_find_entity("view_showcase");
    views[SANDBOX_RESULTS] = views[SANDBOX_LAB];
    if (!app_root || !views[0] || !views[1] || !views[2]) {
        exit_code = 1;
        return false;
    }
    tether_bind_event(tether_registry_find_entity("app_navigation"), TETHER_EVENT_CHANGE, navigate, NULL);
    tether_bind_event(tether_registry_find_entity("RefreshBtn"), TETHER_EVENT_CLICK, on_refresh_clicked, NULL);

    if (!sandbox_widgets_zoo_create(views[SANDBOX_WIDGETS])) {
        exit_code = 1;
        return false;
    }

    Tether_GUID lab = tether_yaml_load("UI/lab.yaml", views[SANDBOX_LAB]);
    if (lab == TETHER_INVALID_GUID || !lab_bind()) {
        exit_code = 1;
        return false;
    }
    sandbox_mount_scene(lab);
    if (!results_bind()) {
        exit_code = 1;
        return false;
    }
    tether_widget_show(app_root);
    return true;
}

static void init(void) {
    tether_font_register("Roboto-Regular", "Font/Roboto-Regular.ttf");
    if (!create_views()) {
        exit_code = 1;
        chitra_quit();
        return;
    }
    if (initial_result[0]) {
        results_load(initial_result);
        wanted = SANDBOX_RESULTS;
    }
    if (smoke || smoke_detach || smoke_invalid || smoke_report) {
        wanted = SANDBOX_LAB;
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
    if (refresh_requested) {
        refresh_requested = false;
        sandbox_showcase_cleanup();
        sandbox_widgets_zoo_cleanup();
        Tether_GUID root = tether_registry_find_entity("sandbox_root");
        if (tether_ecs_is_valid(root)) tether_widget_destroy(root);
        showcase_loaded = false;
        current = -1;
        if (!create_views()) {
            exit_code = 1;
            chitra_quit();
            return;
        }
    }
    sandbox_showcase_update();
    lab_update();
    switch_view();
    if (smoke && tool_now_ms() > deadline) {
        exit_code = 1;
        chitra_quit();
    }
}

static void after_frame(void) {
    static unsigned smoke_stage;
    static Tether_GUID old_showcase_root;
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
        if (!sandbox_showcase_check()) {
            fprintf(stderr, "Showcase interaction check failed\n");
            exit_code = 1;
        }
        old_showcase_root = root;
        tether_ui_set_segment_selected(tether_registry_find_entity("showcase_tabs"), 2);
        tether_dispatch_event(tether_registry_find_entity("showcase_preset_layout"), TETHER_EVENT_CLICK);
        tether_dispatch_event(tether_registry_find_entity("showcase_show_toast"), TETHER_EVENT_CLICK);
        smoke_stage = 2;
        return;
    }
    if (smoke && smoke_stage == 2 && current == SANDBOX_SHOWCASE) {
        Tether_GUID canvas = tether_registry_find_entity("showcase_preview");
        Tether_SlotTransform *canvas_slot = tether_ecs_get_component(canvas, TETHER_COMPONENT_SLOT_TRANSFORM);
        Tether_GUID wrapper = tether_widget_get_child_at(canvas, 0);
        Tether_GUID content = tether_widget_get_child_at(wrapper, 0);
        Tether_SlotTransform *wrapper_slot = tether_ecs_get_component(wrapper, TETHER_COMPONENT_SLOT_TRANSFORM);
        Tether_SlotTransform *content_slot = tether_ecs_get_component(content, TETHER_COMPONENT_SLOT_TRANSFORM);
        Tether_GUID toast = tether_registry_find_entity("showcase_toast");
        Tether_GUID toast_panel = tether_widget_get_child_at(toast, 0);
        Tether_SlotTransform *toast_slot = tether_ecs_get_component(toast_panel, TETHER_COMPONENT_SLOT_TRANSFORM);
        if (!canvas_slot || canvas_slot->width < 400 || canvas_slot->height < 200 ||
            !wrapper_slot || !content_slot ||
            content_slot->width < wrapper_slot->width - 1 || content_slot->height < wrapper_slot->height - 1 ||
            !toast_slot || toast_slot->width < 300 || toast_slot->height < 40) {
            fprintf(stderr, "Showcase preview or toast geometry failed\n");
            if (canvas_slot) fprintf(stderr, "Canvas: %.1f x %.1f\n", canvas_slot->width, canvas_slot->height);
            if (toast_slot) fprintf(stderr, "Toast: %.1f x %.1f\n", toast_slot->width, toast_slot->height);
            exit_code = 1;
        }
        tether_dispatch_event(tether_registry_find_entity("showcase_toast_close"), TETHER_EVENT_CLICK);
        tether_ui_set_segment_selected(tether_registry_find_entity("showcase_tabs"), 1);
        tether_dispatch_event(tether_registry_find_entity("showcase_open_dialog"), TETHER_EVENT_CLICK);
        smoke_stage = 3;
        return;
    }
    if (smoke && smoke_stage == 3 && current == SANDBOX_SHOWCASE) {
        Tether_GUID dialog = tether_registry_find_entity("showcase_dialog");
        Tether_SlotTransform *dialog_slot = tether_ecs_get_component(dialog, TETHER_COMPONENT_SLOT_TRANSFORM);
        if (!dialog_slot || dialog_slot->width < 1000 || dialog_slot->height < 700 ||
            tether_hit_test(5.0f, 5.0f) != dialog) {
            fprintf(stderr, "Showcase dialog geometry failed\n");
            exit_code = 1;
        }
        Tether_GUID example = tether_registry_find_entity("example_target");
        Tether_GUID cards = tether_registry_find_entity("example_card_row");
        Tether_SlotTransform *example_slot = tether_ecs_get_component(example, TETHER_COMPONENT_SLOT_TRANSFORM);
        Tether_SlotTransform *cards_slot = tether_ecs_get_component(cards, TETHER_COMPONENT_SLOT_TRANSFORM);
        if (!example_slot || !cards_slot || example_slot->height < 120 || cards_slot->height < 40 ||
            cards_slot->y + cards_slot->height > example_slot->y + example_slot->height + 1) {
            fprintf(stderr, "Showcase example geometry failed\n");
            exit_code = 1;
        }
        tether_dispatch_event(tether_registry_find_entity("showcase_dialog_close"), TETHER_EVENT_CLICK);
        tether_dispatch_event(tether_registry_find_entity("RefreshBtn"), TETHER_EVENT_CLICK);
        smoke_stage = 4;
        return;
    }
    if (smoke && smoke_stage == 4 && current == SANDBOX_SHOWCASE) {
        Tether_GUID root = tether_registry_find_entity("AppRoot");
        Tether_Hierarchy *hierarchy = tether_ecs_get_component(root, TETHER_COMPONENT_HIERARCHY);
        if (!root || root == old_showcase_root || !hierarchy ||
            hierarchy->parent != views[SANDBOX_SHOWCASE] ||
            !tether_registry_find_entity("RefreshBtn")) {
            exit_code = 1;
        }
        smoke_stage = 5;
        sandbox_navigate(SANDBOX_WIDGETS);
        return;
    }
    if (smoke && smoke_stage == 5 && current == SANDBOX_WIDGETS) {
        if (!sandbox_widgets_zoo_check()) exit_code = 1;
        smoke_stage = 6;
        sandbox_navigate(SANDBOX_RESULTS);
        return;
    }
    if (smoke && lab_smoke_status() >= 0 && current == SANDBOX_RESULTS) {
        if (lab_smoke_status()) {
            exit_code = lab_smoke_status();
        }
        if (!results_has_geometry()) {
            fprintf(stderr, "Results geometry check failed\n");
            exit_code = 1;
        }
        if (!lab_has_geometry()) {
            fprintf(stderr, "Lab geometry check failed\n");
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

int main(int argc, char **argv) {
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
    Tether_Config config = {0};
    config.width = 1200;
    config.height = 880;
    config.title = "Tether Sandbox";
    config.target_fps = 60;
    config.on_init = init;
    config.on_frame = update;
    config.on_post_frame = after_frame;
    config.on_term = lab_cleanup;
    tether_run(&config);
    return exit_code;
}
