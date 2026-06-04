#define SOKOL_IMPL
#define SOKOL_NO_ENTRY

#if defined(_WIN32)
    #define SOKOL_D3D11
#elif defined(__APPLE__)
    #define SOKOL_METAL
#else
    #define SOKOL_GLCORE
#endif

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "tether.h"

static sg_pass_action pass_action;

static void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });
    
    pass_action = (sg_pass_action) {
        .colors[0] = { 
            .load_action = SG_LOADACTION_CLEAR, 
            .clear_value = { 0.15f, 0.15f, 0.15f, 1.0f } 
        }
    };
}

static void frame(void) {
    sg_begin_pass(&(sg_pass){ 
        .action = pass_action, 
        .swapchain = sglue_swapchain() 
    });
    sg_end_pass();
    sg_commit();
}

static void cleanup(void) {
    sg_shutdown();
}

void tether_run(Tether_App_Config* config) {
    sapp_desc desc = {0};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.width = config->width;
    desc.height = config->height;
    desc.window_title = config->title;
    desc.logger.func = slog_func;
    
    sapp_run(&desc);
}