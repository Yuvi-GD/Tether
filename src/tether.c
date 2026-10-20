#include "tether/tether.h"
#include "tether/backends/tether_backend.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_registry.h"
#include "tether/engine/tether_engine.h"
#include "tether/ui/tether_widgets.h"

void tether_post_init(void)
{
    tether_ecs_register_component_static(TETHER_COMPONENT_SHAPE, sizeof(Tether_Shape));
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
}

void tether_run(Tether_App_Config* config) 
{
    Tether_EngineConfig engine_config = 
    {
        .on_init = config ? config->on_init : (void(*)(void))0,
        .on_frame = config ? config->on_frame : (void(*)(void))0,
        .on_post_frame = config ? config->on_post_frame : (void(*)(void))0,
        .on_term = config ? config->on_term : (void(*)(void))0,
        .initial_yaml = config ? config->initial_yaml : (const char*)0
    };
    
    tether_engine_init(&engine_config);

    tether_post_init();
    
    /* Hand control over to the backend to start the OS window loop */
    tether_engine_run(config);
}
