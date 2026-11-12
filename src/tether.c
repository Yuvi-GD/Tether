#include "tether/tether.h"
#include "tether/backends/tether_backend.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_registry.h"
#include "tether/core/tether_input.h"
#include "tether/engine/tether_engine.h"
#include "tether/engine/parser/tether_yaml.h"
#include "tether/ui/tether_widgets.h"
#include "tether/ui/tether_ui.h"
#include "tether/engine/parser/tether_yaml_widgets.h"
#include <stddef.h>

static bool g_redraw_requested = true;
static bool g_frame_drawn = false;
static Tether_Config g_config = {0};

/* The single unified initialization function */
void tether_init(Tether_Config* config) {
    if (config) {
        g_config = *config;
    }
    
    tether_ecs_init();
    
    /* Core spatial, hierarchy, and state components */
    tether_ecs_register_component_static(TETHER_COMPONENT_SLOT_TRANSFORM, sizeof(Tether_SlotTransform), TETHER_DEFAULT_SLOT_TRANSFORM);
    tether_ecs_register_component_static(TETHER_COMPONENT_RENDER_TRANSFORM, sizeof(Tether_RenderTransform), TETHER_DEFAULT_RENDER_TRANSFORM);
    tether_ecs_register_component_static(TETHER_COMPONENT_HIERARCHY, sizeof(Tether_Hierarchy), NULL);
    tether_ecs_register_component_static(TETHER_COMPONENT_VISIBILITY, sizeof(Tether_Visibility), TETHER_DEFAULT_VISIBILITY);
    tether_ecs_register_component_static(TETHER_COMPONENT_LAYOUT, sizeof(Tether_Layout), TETHER_DEFAULT_LAYOUT);

    tether_ecs_register_component_static(TETHER_COMPONENT_DIRTY_LAYOUT, 0, NULL);
    tether_ecs_register_component_static(TETHER_COMPONENT_DIRTY_VISUAL, 0, NULL);
    tether_ecs_register_component_static(TETHER_COMPONENT_DIRTY_HIERARCHY, 0, NULL);
    tether_ecs_register_component_static(TETHER_COMPONENT_DIRTY_TEXT_CONTENT, 0, NULL);
    tether_ecs_register_component_static(TETHER_COMPONENT_VOLATILE, 0, NULL);
    
    /* Root entities and registries */
    tether_ecs_init_roots();
    tether_registry_init();
    
    /* Visual, layout slot, and interaction components */
    tether_ecs_register_component_static(TETHER_COMPONENT_SHAPE, sizeof(Tether_Shape), TETHER_DEFAULT_SHAPE);
    tether_ecs_register_component_static(TETHER_COMPONENT_ANCHOR_SLOT, sizeof(Tether_AnchorSlot), TETHER_DEFAULT_ANCHOR_SLOT);
    tether_ecs_register_component_static(TETHER_COMPONENT_FLEX_SLOT, sizeof(Tether_FlexSlot), TETHER_DEFAULT_FLEX_SLOT);
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT, sizeof(Tether_Text), TETHER_DEFAULT_TEXT);
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_WORD, sizeof(Tether_TextWord), NULL);
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_LABEL, sizeof(Tether_TextLabel), NULL);
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_PARAGRAPH, sizeof(Tether_TextParagraph), NULL);
    tether_ecs_register_component_static(TETHER_COMPONENT_TEXT_DYNAMIC, sizeof(Tether_TextDynamic), NULL);
    tether_ecs_register_component_static(TETHER_COMPONENT_OVERFLOW, sizeof(Tether_Overflow), TETHER_DEFAULT_OVERFLOW);
    tether_ecs_register_component_static(TETHER_COMPONENT_IMAGE, sizeof(Tether_Image), NULL);
    tether_ecs_register_component_static(TETHER_COMPONENT_INTERACTABLE, sizeof(Tether_Interactable), TETHER_DEFAULT_INTERACTABLE);
    tether_ecs_register_component_static(TETHER_COMPONENT_IS_LEAF, 0, NULL);

    /* Semantic UI State Components (Pure ECS) */
    tether_ecs_register_component_static(TETHER_COMPONENT_VALUE_RANGE, sizeof(Tether_ValueRange), TETHER_DEFAULT_VALUE_RANGE);
    tether_ecs_register_component_static(TETHER_COMPONENT_BOOL_STATE, sizeof(Tether_BoolState), TETHER_DEFAULT_BOOL_STATE);
    tether_ecs_register_component_static(TETHER_COMPONENT_SELECTION_STATE, sizeof(Tether_SelectionState), TETHER_DEFAULT_SELECTION_STATE);
    
    /* Built-in widget factories */
    tether_register_widget("Container", tether_widget_create_container);
    tether_register_widget("Panel", tether_ui_panel);
    
    tether_register_widget("Text", tether_ui_text);
    tether_register_widget("Button", tether_ui_button);
    tether_register_widget("ProgressBar", tether_ui_progress_bar);
    tether_register_widget("Slider", tether_ui_slider);
    tether_register_widget("Toggle", tether_ui_toggle);
    tether_register_widget("Checkbox", tether_ui_checkbox);
    tether_register_widget("Radio", tether_ui_radio);
    tether_register_widget("SegmentedBar", tether_ui_segmented_bar);
    tether_register_widget("Menu", tether_ui_menu);
    tether_register_widget("Dropdown", tether_ui_dropdown);
    tether_register_widget("ContextMenu", tether_ui_context_menu);
    tether_register_widget("Collapsible", tether_ui_collapsible);
    tether_register_widget("Switcher", tether_ui_switcher);
}

void tether_on_init(void) {
    tether_engine_init();

    /* Load initial YAML after backend is ready (g_backend must be valid for tether_engine_realize) */
    if (g_config.initial_yaml) {
        tether_yaml_load(g_config.initial_yaml, TETHER_INVALID_GUID);
    }

    if (g_config.on_init) {
        g_config.on_init();
    }
    
    /* Load fonts into backend after user registration */
    for (uint32_t i = 0; i < 32; ++i) {
        const char* name = tether_font_get_name(i);
        const char* path = tether_font_get_path(i);
        if (path) g_backend.text->load_font(name ? name : "default", path);
    }
}

static int s_last_w = 0, s_last_h = 0;
void* tether_on_frame(int width, int height) {
    bool resized = (width != s_last_w || height != s_last_h);
    if (resized) {
        s_last_w = width;
        s_last_h = height;
    }
    
    uint32_t res_x = 0, res_y = 0;
    g_backend.system->get_resolution(&res_x, &res_y);
    
    if (g_config.on_frame) {
        g_config.on_frame();
    }
    
    /* Only render if needed (resized, input received, or components are dirty) */
    g_frame_drawn = tether_engine_frame((float)res_x, (float)res_y, resized || g_redraw_requested);
    g_redraw_requested = false;
    
    return NULL;
}

void tether_on_post_frame(void) {
    if (g_config.on_post_frame) {
        g_config.on_post_frame();
    }
}

bool tether_did_draw(void) {
    return g_frame_drawn;
}

void tether_queue_redraw(void) {
    g_redraw_requested = true;
}

void tether_on_input(const Tether_Pointer_Event* event) {
    tether_input_process_event(event);
    tether_queue_redraw();
}

void tether_on_cleanup(void) {
    if (g_config.on_term) {
        g_config.on_term();
    }

    /* Terminate engine scene */
    tether_engine_term();

    tether_yaml_term();

    /* Teardown engine-specific resources before backend shuts down */
    tether_registry_term();
    tether_ecs_term();
}

void tether_run(Tether_Config* config) 
{
    tether_init(config);
    
    /* Hand control over to the backend to start the OS window loop */
    g_backend.system->run(config);
}
