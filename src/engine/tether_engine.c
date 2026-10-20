#include "tether/engine/tether_engine.h"
#include "tether/engine/tether_render.h"
#include "tether/backends/tether_backend.h"
#include "tether/core/tether_registry.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/engine/parser/tether_yaml.h"
#include <stddef.h>

static bool g_redraw_requested = true;
static bool g_frame_drawn = false;
static Tether_EngineConfig g_engine_config = {0};

void tether_engine_init(Tether_EngineConfig* config) {
    if (config) {
        g_engine_config = *config;
    }
    
    tether_ecs_init();
    
    tether_ecs_register_component_static(TETHER_COMPONENT_SLOT_TRANSFORM, sizeof(Tether_SlotTransform));
    tether_ecs_register_component_static(TETHER_COMPONENT_RENDER_TRANSFORM, sizeof(Tether_RenderTransform));
    tether_ecs_register_component_static(TETHER_COMPONENT_HIERARCHY, sizeof(Tether_Hierarchy));
    tether_ecs_register_component_static(TETHER_COMPONENT_VISIBILITY, sizeof(Tether_Visibility));
    tether_ecs_register_component_static(TETHER_COMPONENT_LAYOUT, sizeof(Tether_Layout));

    tether_ecs_register_component_static(TETHER_COMPONENT_DIRTY_LAYOUT, 0);
    tether_ecs_register_component_static(TETHER_COMPONENT_DIRTY_VISUAL, 0);
    tether_ecs_register_component_static(TETHER_COMPONENT_DIRTY_HIERARCHY, 0);
    tether_ecs_register_component_static(TETHER_COMPONENT_VOLATILE, 0);
    
    tether_ecs_init_roots();
    tether_registry_init();  
}

void tether_engine_on_init(void) {
    tether_render_init();

    /* Load initial YAML after RHI is ready (g_rhi must be valid for tether_render_realize) */
    if (g_engine_config.initial_yaml) {
        tether_yaml_load(g_engine_config.initial_yaml, TETHER_INVALID_GUID);
    }

    if (g_engine_config.on_init) {
        g_engine_config.on_init();
    }
    
    /* Load fonts into RHI after user registration */
    for (uint32_t i = 0; i < 32; ++i) {
        const char* name = tether_font_get_name(i);
        const char* path = tether_font_get_path(i);
        if (path) g_rhi.text->load_font(name ? name : "default", path);
    }
}

static int s_last_w = 0, s_last_h = 0;
void* tether_engine_on_frame(int width, int height) {
    bool resized = (width != s_last_w || height != s_last_h);
    if (resized) {
        s_last_w = width;
        s_last_h = height;
    }
    
    uint32_t res_x = 0, res_y = 0;
    g_rhi.system->get_resolution(&res_x, &res_y);
    
    if (g_engine_config.on_frame) {
        g_engine_config.on_frame();
    }
    
    // Only render if needed (resized, input received, or components are dirty)
    g_frame_drawn = tether_render_frame((float)res_x, (float)res_y, resized || g_redraw_requested);
    g_redraw_requested = false;
    
    return NULL;
}

void tether_engine_on_post_frame(void) {
    if (g_engine_config.on_post_frame) {
        g_engine_config.on_post_frame();
    }
}

bool tether_engine_did_draw(void) {
    return g_frame_drawn;
}

void tether_engine_on_input(const Tether_Pointer_Event* event) {
    tether_input_process_event(event);
    tether_engine_queue_redraw();
}

void tether_engine_on_cleanup(void) {
    if (g_engine_config.on_term) {
        g_engine_config.on_term();
    }

    Tether_DenseArray* nodes = tether_ecs_get_dense_array(TETHER_COMPONENT_SLOT_TRANSFORM);
    if (nodes) {
        printf("length of TOTAL ENTITIES %lu\n", nodes->count);
    }

    /* Terminate Renderer */
    tether_render_term();

    /* Teardown engine-specific resources before HAL and RHI shut down. */
    tether_registry_term();
    tether_ecs_term();
}

void tether_engine_queue_redraw(void) {
    g_redraw_requested = true;
}

void tether_engine_run(Tether_App_Config* config)
{
    g_rhi.system->run(config);
}