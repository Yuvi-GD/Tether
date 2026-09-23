#include "sandbox_events.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_registry.h"
#include "tether/ui/tether_widgets.h"
#include "parsers/tether_yaml.h"
#include <stdio.h>
#include <string.h>

void on_close_c_modal(Tether_GUID entity, Tether_EventType type, void* user_data) {
    Tether_GUID root = (Tether_GUID)(uintptr_t)user_data;
    tether_widget_destroy(root);
}

void on_open_c_modal(Tether_GUID entity, Tether_EventType type, void* user_data) {
    /* Spawn it by string name from the widget registry! */
    Tether_GUID modal_root = tether_create_widget("MyCModal", tether_ecs_get_main_root());
    if (modal_root != TETHER_INVALID_GUID) {
        printf("[Sandbox] Opened C Modal dynamically!\n");
        tether_widget_show(modal_root);
    }
}

void on_close_yaml_modal(Tether_GUID entity, Tether_EventType type, void* user_data) {
    Tether_GUID root = (Tether_GUID)(uintptr_t)user_data;
    tether_widget_destroy(root);
}

void on_open_yaml_modal(Tether_GUID entity, Tether_EventType type, void* user_data) {
    /* 1. Load the sub-tree from file dynamically */
    Tether_GUID modal_root = tether_yaml_load("../../examples/UI/modal.yaml", TETHER_INVALID_GUID);
    
    if (modal_root != TETHER_INVALID_GUID) {
        printf("[Sandbox] Opened YAML Modal dynamically!\n");
        /* 2. Bind the close button dynamically */
        Tether_GUID close_btn = tether_ecs_find_by_id("btn_close_yaml_modal");
        if (close_btn != TETHER_INVALID_GUID) {
            tether_bind_event(close_btn, TETHER_EVENT_CLICK, on_close_yaml_modal, (void*)(uintptr_t)modal_root);
        }
    } else {
        printf("[Sandbox] Failed to open YAML Modal!\n");
    }
}

void sandbox_bind_events(void) {
    /* Bind to static sidebar buttons from index.yaml */
    Tether_GUID btn_c_modal = tether_ecs_find_by_id("btn_open_c_modal");
    if (btn_c_modal != TETHER_INVALID_GUID) {
        tether_bind_event(btn_c_modal, TETHER_EVENT_CLICK, on_open_c_modal, NULL);
    }
    
    Tether_GUID btn_yaml_modal = tether_ecs_find_by_id("btn_open_yaml_modal");
    if (btn_yaml_modal != TETHER_INVALID_GUID) {
        tether_bind_event(btn_yaml_modal, TETHER_EVENT_CLICK, on_open_yaml_modal, NULL);
    }
}