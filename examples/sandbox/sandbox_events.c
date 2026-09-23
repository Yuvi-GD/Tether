#include "sandbox_events.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_registry.h"
#include "tether/ui/tether_widgets.h"
#include "parsers/tether_yaml.h"
#include <stdio.h>

/* ========================================================
 * Playground Events
 * ======================================================== */

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

void on_settings_clicked(Tether_GUID entity, Tether_EventType type, void* user_data)
{
    Tether_GUID btn_hide = tether_ecs_find_by_id("btn_hide");
    if(btn_hide != TETHER_INVALID_GUID)
    {
        tether_widget_set_visibility(btn_hide, TETHER_VISIBLE);
        printf("[Sandbox] Settings clicked - Showing btn_hide!\n");
    }
}

void on_hide_clicked(Tether_GUID entity, Tether_EventType type, void* user_data)
{
    tether_widget_set_visibility(entity, TETHER_HIDDEN);
    printf("[Sandbox] btn_hide clicked - Hiding self!\n");
}

void on_collapse_clicked(Tether_GUID entity, Tether_EventType type, void* user_data)
{
    tether_widget_set_visibility(entity, TETHER_COLLAPSED);
    printf("[Sandbox] btn_collapse clicked - Collapsing self!\n");
}

void on_align_clicked(Tether_GUID entity, Tether_EventType type, void* user_data)
{
    Tether_GUID txt = tether_ecs_find_by_id("txt_align");
    if (txt != TETHER_INVALID_GUID)
    {
        Tether_Text* t = (Tether_Text*)tether_ecs_get_component(txt, TETHER_COMPONENT_TEXT);
        Tether_TextWord* tw = (Tether_TextWord*)tether_ecs_get_component(txt, TETHER_COMPONENT_TEXT_WORD);
        if (t && tw) 
        {
            /* Cycle AUTO (0) -> FILL (1) -> START (2) -> CENTER (3) -> END (4) */
            t->align_x = (Tether_Align)((t->align_x + 1) % 5);
            
            const char* str = "Align: AUTO";
            if (t->align_x == TETHER_ALIGN_FILL) str = "Align: FILL";
            else if (t->align_x == TETHER_ALIGN_START) str = "Align: START";
            else if (t->align_x == TETHER_ALIGN_CENTER) str = "Align: CENTER";
            else if (t->align_x == TETHER_ALIGN_END) str = "Align: END";
            
            strcpy(tw->data, str);
            printf("[Sandbox] btn_align clicked - Alignment changed to %s\n", str);
        }
    }
}

/* ========================================================
 * Events Binding
 * ======================================================== */

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

        Tether_GUID settings_btn = tether_ecs_find_by_id("SettingsBtn");
    if (settings_btn != TETHER_INVALID_GUID)
    {
        /* We only bind CLICK here to demo C-event bindings. */
        tether_bind_event(settings_btn, TETHER_EVENT_CLICK, on_settings_clicked, NULL);
        printf("[Sandbox] Bound click event to SettingsBtn (GUID: %llu)\n", (unsigned long long)settings_btn);
    }
    else
    {
        printf("[Sandbox] Warning: Could not find SettingsBtn\n");
    }
    
    Tether_GUID btn_hide = tether_ecs_find_by_id("btn_hide");
    if (btn_hide != TETHER_INVALID_GUID)
    {
        tether_bind_event(btn_hide, TETHER_EVENT_CLICK, on_hide_clicked, NULL);
    }
    
    Tether_GUID btn_collapse = tether_ecs_find_by_id("btn_collapse");
    if (btn_collapse != TETHER_INVALID_GUID) tether_bind_event(btn_collapse, TETHER_EVENT_CLICK, on_collapse_clicked, NULL);
    
    Tether_GUID btn_align = tether_ecs_find_by_id("btn_align");
    if (btn_align != TETHER_INVALID_GUID)
    {
        tether_bind_event(btn_align, TETHER_EVENT_CLICK, on_align_clicked, NULL);
    }
}