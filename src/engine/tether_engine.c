#include "tether/engine/tether_engine.h"
#include "tether/engine/tether_render.h"
#include "tether/backends/tether_rhi.h"
#include "tether/core/tether_registry.h"
#include <stddef.h>

static bool g_redraw_requested = true;
static Tether_EngineConfig g_engine_config = {0};

void tether_engine_init(Tether_EngineConfig* config) {
    if (config) {
        g_engine_config = *config;
    }
}

void tether_engine_on_hal_init(int width, int height, const void* device, const void* instance) {
    /* Claim RHI and Render subsystems */
    tether_rhi_init(width, height, device, instance);
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
    
    tether_render_frame((float)width, (float)height, resized || g_redraw_requested);
    g_redraw_requested = false;
    
    return (void*)tether_rhi_get_texture();
}

void tether_engine_on_hal_input(const Tether_Pointer_Event* event) {
    tether_input_process_event((Tether_Pointer_Event*)event);
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

void tether_engine_term(void) {
    /* Teardown engine subsystems */
}