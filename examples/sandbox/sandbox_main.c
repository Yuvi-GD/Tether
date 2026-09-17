#include "tether/tether.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_components.h"
#include <stdio.h>

void on_settings_clicked(Tether_GUID entity, Tether_EventType type, void* user_data)
{
    Tether_GUID btn_hide = tether_ecs_find_by_id("btn_hide");
    if(btn_hide != TETHER_INVALID_GUID)
    {
        Tether_LayoutNode* btn_hide_node = (Tether_LayoutNode*)tether_ecs_get_component(btn_hide, TETHER_COMPONENT_LAYOUT_NODE);
        btn_hide_node->visibility = TETHER_VISIBLE;
        printf("[Sandbox] Settings clicked - Showing btn_hide!\n");
    }
}

void on_hide_clicked(Tether_GUID entity, Tether_EventType type, void* user_data)
{
    Tether_LayoutNode* node = (Tether_LayoutNode*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT_NODE);
    if (node) 
    {
        node->visibility = TETHER_HIDDEN;
        printf("[Sandbox] btn_hide clicked - Hiding self!\n");
    }
}

void on_collapse_clicked(Tether_GUID entity, Tether_EventType type, void* user_data)
{
    Tether_LayoutNode* node = (Tether_LayoutNode*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT_NODE);
    if (node) 
    {
        node->visibility = TETHER_COLLAPSED;
        printf("[Sandbox] btn_collapse clicked - Collapsing self!\n");
    }
}

void on_align_clicked(Tether_GUID entity, Tether_EventType type, void* user_data)
{
    Tether_GUID txt = tether_ecs_find_by_id("txt_align");
    if (txt != TETHER_INVALID_GUID)
    {
        Tether_TextStyle* t = (Tether_TextStyle*)tether_ecs_get_component(txt, TETHER_COMPONENT_TEXT_STYLE);
        Tether_TextWord* tw = (Tether_TextWord*)tether_ecs_get_component(txt, TETHER_COMPONENT_TEXT_WORD);
        if (t && tw) 
        {
            t->align_x = (Tether_Align)((t->align_x + 1) % 4); /* Cycle FILL -> START -> CENTER -> END */
            
            const char* str = "Align: FILL";
            if (t->align_x == TETHER_ALIGN_START) str = "Align: START";
            else if (t->align_x == TETHER_ALIGN_CENTER) str = "Align: CENTER";
            else if (t->align_x == TETHER_ALIGN_END) str = "Align: END";
            
            strcpy(tw->data, str);
            printf("[Sandbox] btn_align clicked - Alignment changed to %s\n", str);
        }
    }
}

void sandbox_init(void)
{
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

int main()
{
    Tether_App_Config config = {
        .width = 1024,
        .height = 768,
        .title = "Tether UI Sandbox",
        .initial_yaml = "../../examples/UI/index.yaml",
        
        /* Explicitly define our engine behavior */
        .memory_strategy = TETHER_MEMORY_HIGH_WATER_MARK,
        .renderer = TETHER_RENDERER_WEBGPU,
        .target_fps = 0, /* VSync */
        
        .on_init = sandbox_init
    };
    tether_font_register("Roboto-Regular", "../../examples/Font/Roboto-Regular.ttf");
    
    tether_run(&config);
    return 0;
}