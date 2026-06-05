#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#define SOKOL_GLCORE 

/* 1. Tell the ThorVG headers that we built the GL backend */
#define THORVG_GL_RASTER_SUPPORT 1

#include "sokol_app.h"
#include "thorvg_capi.h"
#include "tether.h"

/* 2. In ThorVG 1.0.5, Tvg_Canvas and Tvg_Paint are ALREADY pointers. 
 * We remove the '*' to prevent double-indirection errors. */
static Tvg_Canvas tvg_canvas = NULL;
static int current_width = 0;
static int current_height = 0;

static void init(void) {
    /* Initialize ThorVG's Hardware Engine */
    tvg_engine_init(1);
    
    /* Create the Hardware Canvas */
    tvg_canvas = tvg_glcanvas_create(TVG_ENGINE_OPTION_NONE);
    
    /* Link ThorVG to the window Sokol just opened */
    current_width = sapp_width();
    current_height = sapp_height();
    
    /* The '0' ID targets the default application window frame buffer */
    tvg_glcanvas_set_target(tvg_canvas, NULL, NULL, NULL, 0, current_width, current_height, TVG_COLORSPACE_ABGR8888S);
}

static void frame(void) {
    /* Update ThorVG's canvas target if the user resizes the window */
    if (current_width != sapp_width() || current_height != sapp_height()) {
        current_width = sapp_width();
        current_height = sapp_height();
        tvg_glcanvas_set_target(tvg_canvas, NULL, NULL, NULL, 0, current_width, current_height, TVG_COLORSPACE_ABGR8888S);
    }

    tvg_canvas_remove(tvg_canvas, NULL);
    
    /* Draw the Dark Gray Background */
    Tvg_Paint bg = tvg_shape_new(); /* Note: No asterisk */
    tvg_shape_append_rect(bg, 0, 0, current_width, current_height, 0, 0, true);
    tvg_shape_set_fill_color(bg, 38, 38, 38, 255);
    tvg_canvas_add(tvg_canvas, bg);

    /* Draw the "Hello World" Red Triangle */
    Tvg_Paint triangle = tvg_shape_new(); /* Note: No asterisk */
    tvg_shape_move_to(triangle, current_width / 2.0f, 100.0f);                      
    tvg_shape_line_to(triangle, current_width / 2.0f + 200.0f, current_height - 100.0f); 
    tvg_shape_line_to(triangle, current_width / 2.0f - 200.0f, current_height - 100.0f); 
    tvg_shape_close(triangle);                                                      
    tvg_shape_set_fill_color(triangle, 255, 50, 50, 255);
    
    tvg_canvas_add(tvg_canvas, triangle);

    /* Execute the GPU Math and Push to Screen */
    tvg_canvas_draw(tvg_canvas, false);
    tvg_canvas_sync(tvg_canvas);
}

static void cleanup(void) {
    tvg_canvas_destroy(tvg_canvas);
    tvg_engine_term();
}

void tether_run(Tether_App_Config* config) {
    sapp_desc desc = {0};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.width = config->width;
    desc.height = config->height;
    desc.window_title = config->title;
    
    sapp_run(&desc);
}