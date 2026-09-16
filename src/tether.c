#include "tether/tether.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "backends/tether_hal.h"
#include "parsers/tether_yaml.h"


void tether_run(Tether_App_Config* config) {
    /* Initialize the Tether UI Kernel (Memory Arena, Event Dispatcher, etc.) */
    tether_ecs_init();
    
    tether_ecs_register_component_type(TETHER_COMPONENT_SLOT_TRANSFORM, sizeof(Tether_SlotTransform));
    tether_ecs_register_component_type(TETHER_COMPONENT_RENDER_TRANSFORM, sizeof(Tether_RenderTransform));
    tether_ecs_register_component_type(TETHER_COMPONENT_STYLE, sizeof(Tether_Style));
    tether_ecs_register_component_type(TETHER_COMPONENT_HIERARCHY, sizeof(Tether_Hierarchy));
    tether_ecs_register_component_type(TETHER_COMPONENT_LAYOUT_NODE, sizeof(Tether_LayoutNode));
    tether_ecs_register_component_type(TETHER_COMPONENT_ANCHOR_SLOT, sizeof(Tether_AnchorSlot));
    tether_ecs_register_component_type(TETHER_COMPONENT_FLEX_SLOT, sizeof(Tether_FlexSlot));
    tether_ecs_register_component_type(TETHER_COMPONENT_TEXT_STYLE, sizeof(Tether_TextStyle));
    tether_ecs_register_component_type(TETHER_COMPONENT_TEXT_WORD, sizeof(Tether_TextWord));
    tether_ecs_register_component_type(TETHER_COMPONENT_TEXT_LABEL, sizeof(Tether_TextLabel));
    tether_ecs_register_component_type(TETHER_COMPONENT_TEXT_PARAGRAPH, sizeof(Tether_TextParagraph));
    tether_ecs_register_component_type(TETHER_COMPONENT_TEXT_DYNAMIC, sizeof(Tether_TextDynamic));
    tether_ecs_register_component_type(TETHER_COMPONENT_CLIP_MASK, sizeof(Tether_ClipMask));
    tether_ecs_register_component_type(TETHER_COMPONENT_IMAGE, sizeof(Tether_Image));
    tether_ecs_register_component_type(TETHER_COMPONENT_INTERACTABLE, sizeof(Tether_Interactable));
    
    if (config->initial_yaml) {
        tether_yaml_load(config->initial_yaml);
    }
    
    if (config->on_init) {
        config->on_init();
    }

    /* Hand control over to the Hardware Abstraction Layer to start the OS window loop */
    tether_hal_run(config);
    
    tether_ecs_term();
}
