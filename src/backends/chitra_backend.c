/* =============================================================================
 * chitra_backend.c
 *
 * Concrete implementation of the Tether backend interface backed by Chitra.
 * This is the ONLY file in Tether that knows about Chitra.
 * ============================================================================= */

#include "tether/backends/tether_backend.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_input.h"
#include "chitra.h"
#include <stdio.h>

/* Tether engine lifecycle hooks called by this backend. */
extern void  tether_engine_on_init(void);
extern void* tether_engine_on_frame(int w, int h);
extern void  tether_engine_on_post_frame(void);
extern void  tether_engine_on_cleanup(void);
extern void  tether_engine_on_input(const Tether_Pointer_Event* e);

/* =============================================================================
 * Chitra Callbacks (on_init, on_frame, on_post_frame, on_event, on_cleanup)
 * ============================================================================= */

static void on_init(void) {
    tether_engine_on_init();
}

static void on_frame(void) {
    int w = chitra_width();
    int h = chitra_height();
    if (w <= 0 || h <= 0) return;
    tether_engine_on_frame(w, h);
}

static void on_post_frame(void) {
    tether_engine_on_post_frame();
}

static void on_cleanup(void) {
    tether_engine_on_cleanup();
}

static void on_event(const Chitra_InputEvent* e) {
    if (e->type == CHITRA_EVENT_MOUSE_DOWN ||
        e->type == CHITRA_EVENT_MOUSE_UP   ||
        e->type == CHITRA_EVENT_MOUSE_MOVE) {

        Tether_Pointer_Event pe = {0};

        if      (e->type == CHITRA_EVENT_MOUSE_DOWN) pe.type = TETHER_POINTER_DOWN;
        else if (e->type == CHITRA_EVENT_MOUSE_UP)   pe.type = TETHER_POINTER_UP;
        else                                          pe.type = TETHER_POINTER_MOVE;

        pe.x = e->mouse_x;
        pe.y = e->mouse_y;

        if      (e->button == 0) pe.button = TETHER_MOUSE_BUTTON_LEFT;
        else if (e->button == 1) pe.button = TETHER_MOUSE_BUTTON_RIGHT;
        else if (e->button == 2) pe.button = TETHER_MOUSE_BUTTON_MIDDLE;
        else                     pe.button = TETHER_MOUSE_BUTTON_NONE;

        tether_engine_on_input(&pe);
    }
}

/* =============================================================================
 * Tether_System_API  Implementation
 * ============================================================================= */

static void sys_get_size(uint32_t* out_w, uint32_t* out_h) {
    if (out_w) *out_w = (uint32_t)chitra_width();
    if (out_h) *out_h = (uint32_t)chitra_height();
}

static void sys_set_resolution(uint32_t res_x, uint32_t res_y) {
    chitra_set_resolution((int)res_x, (int)res_y);
}

static void sys_get_resolution(uint32_t* out_x, uint32_t* out_y) {
    int rx = 0, ry = 0;
    chitra_get_resolution(&rx, &ry);
    if (out_x) *out_x = (uint32_t)rx;
    if (out_y) *out_y = (uint32_t)ry;
}

/* =============================================================================
 * Tether_Shape_API  Implementation
 * ============================================================================= */

static uint32_t shape_create(void) {
    return chitra_create(CHITRA_PIPELINE_SHAPE);
}

/* =============================================================================
 * Tether_Text_API  Implementation
 * ============================================================================= */

static void text_measure_text(const char* text, uint32_t font_id, float size, float wrap_width,
                              float* out_w, float* out_h) {
    if (wrap_width > 0.0f) {
        chitra_measure_text_wrapped((int)font_id, size, text, wrap_width, out_w, out_h);
    } else {
        chitra_measure_text((int)font_id, size, text, out_w, out_h);
    }
}

static uint32_t text_create(void) {
    return chitra_create(CHITRA_PIPELINE_TEXT);
}

static bool text_set_text(uint32_t handle, uint32_t font_id, float size, const char* text, float wrap_width, float x, float y) {
    if (wrap_width > 0.0f) {
        return chitra_text_set_wrapped(handle, (int)font_id, size, text, wrap_width, x, y);
    } else {
        return chitra_text_set(handle, (int)font_id, size, text, x, y);
    }
}

/* =============================================================================
 * Backend Entry Point
 * ============================================================================= */

static void backend_run(Tether_App_Config* config) {
    Chitra_Config chitra_cfg = {
        .width         = config ? config->width      : 1280,
        .height        = config ? config->height     : 720,
        .title         = config ? config->title      : "Tether",
        .target_fps    = config ? config->target_fps : 0,  /* VSync default */
        .on_init       = on_init,
        .on_frame      = on_frame,
        .on_post_frame = on_post_frame,
        .on_event      = on_event,
        .on_cleanup    = on_cleanup,
    };

    chitra_run(&chitra_cfg);
}

/* =============================================================================
 * Static API Tables + Global g_backend
 * ============================================================================= */

static Tether_System_API s_sys_api = {
    .run               = backend_run,
    .get_size          = sys_get_size,
    .set_resolution    = sys_set_resolution,
    .get_resolution    = sys_get_resolution,
    .mark_dirty        = chitra_mark_dirty,
    .register_clip     = chitra_register_clip,
    .update_clip       = chitra_update_clip,
    .clip_intersect    = chitra_clip_intersect,
    .clip_intersect_to = chitra_clip_intersect_to,
    .set_draw_order    = chitra_set_draw_order,
    .set_clip          = chitra_set_clip,
    .get_clip_id       = chitra_get_clip_id,
    .release_clip      = chitra_release_clip,
    .is_alive          = chitra_is_alive,
    .set_alive         = chitra_set_alive,
};

static Tether_Shape_API s_shape_api = {
    .create           = shape_create,
    .destroy          = chitra_destroy,
    .is_alive         = chitra_is_alive,
    .set_bounds       = chitra_rect_set_bounds,
    .set_radius       = chitra_rect_set_radius,
    .set_border       = chitra_rect_set_border,
    .set_color        = chitra_rect_set_color,
    .set_fill_color   = chitra_rect_set_fill_color,
    .set_border_color = chitra_rect_set_border_color,
    .set_opacity      = chitra_rect_set_opacity,
    .set_rotation     = chitra_rect_set_rotation,
};

static Tether_Text_API s_text_api = {
    .load_font        = chitra_load_font,
    .unload_font      = chitra_unload_font,
    .measure_text     = text_measure_text,
    .create           = text_create,
    .destroy          = chitra_destroy,
    .is_alive         = chitra_is_alive,
    .set_text         = text_set_text,
    .set_color        = chitra_text_set_color,
    .set_pos          = chitra_text_set_pos,
    .set_opacity      = chitra_text_set_opacity,
    .set_rotation     = chitra_text_set_rotation,
    .set_scale        = chitra_text_set_scale,
    .get_bounds       = chitra_text_get_bounds,
};

/* The global Backend instance — engine reads this via extern */
Tether_Backend g_backend = {
    .system = &s_sys_api,
    .shape  = &s_shape_api,
    .text   = &s_text_api,
};
