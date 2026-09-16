#include "tether/tether.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_components.h"
#include <stdio.h>

void on_settings_clicked(Tether_GUID entity, Tether_EventType type, void* user_data) {
    printf("[Sandbox] Settings button CLICKED!\n");
}

void sandbox_init(void) {
    Tether_GUID settings_btn = tether_ecs_find_by_id("SettingsBtn");
    if (settings_btn != TETHER_INVALID_GUID) {
        /* We only bind CLICK here to demo C-event bindings. */
        tether_bind_event(settings_btn, TETHER_EVENT_CLICK, on_settings_clicked, NULL);
        printf("[Sandbox] Bound click event to SettingsBtn (GUID: %llu)\n", (unsigned long long)settings_btn);
    } else {
        printf("[Sandbox] Warning: Could not find SettingsBtn\n");
    }
}

int main() {
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