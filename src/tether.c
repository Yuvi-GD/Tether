#include "tether/tether.h"
#include "tether/backends/tether_hal.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_registry.h"
#include "tether/engine/parser/tether_yaml.h"
#include "tether/engine/tether_engine.h"
#include "tether/ui/tether_widgets.h"

void tether_run(Tether_App_Config* config) {
    Tether_EngineConfig engine_config = {
        .on_init = config->on_init,
        .on_frame = config->on_frame,
        .max_render_width = config->max_render_width,
        .max_render_height = config->max_render_height
    };
    tether_engine_init(&engine_config);

    tether_ecs_register_component_static(TETHER_COMPONENT_STYLE, sizeof(Tether_Style));
    tether_ecs_register_component_static(TETHER_COMPONENT_ANCHOR_SLOT, sizeof(Tether_AnchorSlot));
    tether_ecs_register_component_static(TETHER_COMPONENT_FLEX_SLOT, sizeof(Tether_FlexSlot));
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT, sizeof(Tether_Text));
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_WORD, sizeof(Tether_TextWord));
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_LABEL, sizeof(Tether_TextLabel));
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_PARAGRAPH, sizeof(Tether_TextParagraph));
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_DYNAMIC, sizeof(Tether_TextDynamic));
    tether_ecs_register_component_static(TETHER_COMPONENT_OVERFLOW, sizeof(Tether_Overflow));
    tether_ecs_register_component_static(TETHER_COMPONENT_IMAGE, sizeof(Tether_Image));
    tether_ecs_register_component_static(TETHER_COMPONENT_INTERACTABLE, sizeof(Tether_Interactable));
    tether_ecs_register_component_static(TETHER_COMPONENT_IS_LEAF, 0);
    
    tether_register_widget("Panel", tether_widget_create_panel);
    tether_register_widget("Text", tether_widget_create_text);
    tether_register_widget("Button", tether_widget_create_button);
    
    if (config->initial_yaml) {
        tether_yaml_load(config->initial_yaml, TETHER_INVALID_GUID);
    }
    
    /* Hand control over to the Hardware Abstraction Layer to start the OS window loop */
    tether_hal_run(config);

    tether_engine_term();
}
