#include "tether/engine/tether_engine.h"
#include "tether/engine/tether_render.h"
#include "tether/backends/tether_rhi.h"
#include "tether/core/tether_registry.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
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

void tether_engine_on_hal_init(int width, int height, const void* device, const void* instance) {
    /* Claim RHI and Render subsystems */
    tether_rhi_init(width, height, g_engine_config.max_render_width, g_engine_config.max_render_height, device, instance);
    tether_render_init();

    if (g_engine_config.on_init) {
        g_engine_config.on_init();
    }
    
    /* Load fonts into RHI after user registration */
    for (uint32_t i = 1; i <= 31; ++i) {
        const char* path = tether_font_get_path(i);
        if (path) tether_rhi_load_font(path);
    }
}

static int s_last_w = 0, s_last_h = 0;
void* tether_engine_on_hal_frame(int width, int height) {
    bool resized = (width != s_last_w || height != s_last_h);
    if (resized) {
        s_last_w = width;
        s_last_h = height;
        tether_rhi_resize(width, height);
    }
    
    uint32_t tex_w = 0, tex_h = 0;
    tether_rhi_get_texture_size(&tex_w, &tex_h);
    
    if (g_engine_config.on_frame) {
        g_engine_config.on_frame();
    }
    
    g_frame_drawn = tether_render_frame((float)tex_w, (float)tex_h, resized || g_redraw_requested);
    g_redraw_requested = false;
    
    return (void*)tether_rhi_get_texture();
}

bool tether_engine_did_draw(void) {
    return g_frame_drawn;
}

void tether_engine_on_hal_input(const Tether_Pointer_Event* event) {
    if (s_last_w == 0 || s_last_h == 0) return;
    
    uint32_t tex_w = 0, tex_h = 0;
    tether_rhi_get_texture_size(&tex_w, &tex_h);
    
    Tether_Pointer_Event scaled_event = *event;
    scaled_event.x = (event->x * (float)tex_w) / (float)s_last_w;
    scaled_event.y = (event->y * (float)tex_h) / (float)s_last_h;
    
    tether_input_process_event(&scaled_event);
}

void tether_engine_on_hal_cleanup(void) {
    /* Teardown engine-specific resources if any exist in the future before HAL shuts down. */
}

void tether_engine_on_hal_post_cleanup(void) {
    /* RHI term is safely called AFTER sg_shutdown in HAL. */
    tether_rhi_term();
}

void tether_engine_queue_redraw(void) {
    g_redraw_requested = true;
}

void tether_engine_get_texture_size(uint32_t* out_w, uint32_t* out_h) {
    tether_rhi_get_texture_size(out_w, out_h);
}

void tether_engine_term(void) {
    /* Teardown engine subsystems */
    tether_registry_term();
    tether_ecs_term();
}