#include "tether/tether.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "backends/tether_hal.h"
#include "tether/parsers/tether_yaml.h"
void tether_run(Tether_App_Config* config) {
    /* Initialize the Tether UI Kernel (Memory Arena, Event Dispatcher, etc.) */
    tether_ecs_init();
    
    tether_ecs_register_component_type(TETHER_COMPONENT_TRANSFORM, sizeof(Tether_Transform));
    tether_ecs_register_component_type(TETHER_COMPONENT_COLOR, sizeof(Tether_Color));
    tether_ecs_register_component_type(TETHER_COMPONENT_HIERARCHY, sizeof(Tether_Hierarchy));

    if (config->initial_yaml) {
        tether_yaml_load(config->initial_yaml);
    }

    /* Hand control over to the Hardware Abstraction Layer to start the OS window loop */
    tether_hal_run(config);
    
    tether_ecs_term();
}
