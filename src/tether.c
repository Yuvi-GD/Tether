#include "tether/tether.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "backends/tether_hal.h"
#include "parsers/tether_yaml.h"
#include "tether/core/tether_registry.h"
#include "tether/ui/tether_widgets.h"

void tether_run(Tether_App_Config* config) {
    /* Initialize the Tether UI Kernel (Memory Arena, Event Dispatcher, etc.) */
    tether_ecs_init();
    
    tether_ecs_register_component_static(TETHER_COMPONENT_SLOT_TRANSFORM, sizeof(Tether_SlotTransform));
    tether_ecs_register_component_static(TETHER_COMPONENT_RENDER_TRANSFORM, sizeof(Tether_RenderTransform));
    tether_ecs_register_component_static(TETHER_COMPONENT_HIERARCHY, sizeof(Tether_Hierarchy));
    tether_ecs_register_component_static(TETHER_COMPONENT_VISIBILITY, sizeof(Tether_Visibility));
    tether_ecs_register_component_static(TETHER_COMPONENT_LAYOUT, sizeof(Tether_Layout));
    tether_ecs_register_component_static(TETHER_COMPONENT_STYLE, sizeof(Tether_Style));
    tether_ecs_register_component_static(TETHER_COMPONENT_ANCHOR_SLOT, sizeof(Tether_AnchorSlot));
    tether_ecs_register_component_static(TETHER_COMPONENT_FLEX_SLOT, sizeof(Tether_FlexSlot));
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT, sizeof(Tether_Text));
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_WORD, sizeof(Tether_TextWord));
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_LABEL, sizeof(Tether_TextLabel));
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_PARAGRAPH, sizeof(Tether_TextParagraph));
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_DYNAMIC, sizeof(Tether_TextDynamic));
    tether_ecs_register_component_static(TETHER_COMPONENT_CLIP_MASK, sizeof(Tether_ClipMask));
    tether_ecs_register_component_static(TETHER_COMPONENT_IMAGE, sizeof(Tether_Image));
    tether_ecs_register_component_static(TETHER_COMPONENT_INTERACTABLE, sizeof(Tether_Interactable));
    tether_ecs_register_component_static(TETHER_COMPONENT_IS_LEAF, 0);
    tether_ecs_register_component_static(TETHER_COMPONENT_ID, sizeof(Tether_Id));
    
    tether_ecs_init_roots();
    
    tether_registry_init();
    
    tether_register_widget("Panel", tether_widget_create_panel);
    tether_register_widget("Text", tether_widget_create_text);
    tether_register_widget("Button", tether_widget_create_button);
    
    if (config->initial_yaml) {
        tether_yaml_load(config->initial_yaml, TETHER_INVALID_GUID);
    }
    
    if (config->on_init) {
        config->on_init();
    }

    /* Hand control over to the Hardware Abstraction Layer to start the OS window loop */
    tether_hal_run(config);
    
    tether_registry_term();
    tether_ecs_term();
}
