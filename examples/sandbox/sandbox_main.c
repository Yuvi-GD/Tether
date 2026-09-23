#include "tether/tether.h"
#include "tether/core/tether_ecs.h"
#include "sandbox_widget.h"
#include "sandbox_events.h"
#include "components/user_components.h"
#include <stdio.h>
#include <string.h>

extern Tether_GUID tether_yaml_load(const char* filepath, Tether_GUID parent);

void sandbox_init(void)
{
    // example of user component setup
    // tether_ecs_register_component_static(GAME_COMPONENT_HEALTH, sizeof(Game_Health));
    // tether_ecs_register_component_static(GAME_COMPONENT_RATE, sizeof(Rate));

    // define the limimt of static component which is starting index point of dynamic component
    // if using cusom component
    // tether_ecs_set_static_component_limit(TETHER_COMPONENT_MAX);

    /* Register our custom third-party C Widget! */
    tether_register_widget("MyCModal", create_my_c_modal);
    
    /* Bind all modal events dynamically via the separated file */
    sandbox_bind_events();
}

int main()
{
    Tether_App_Config config = 
    {
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