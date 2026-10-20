#ifndef TETHER_BACKEND_H
#define TETHER_BACKEND_H

#include <stdint.h>
#include <stdbool.h>
#include "tether/tether.h"
#include "tether/core/tether_components.h"

/* ============================================================================
 * Tether Unified Backend Interface (Windowing HAL + Rendering RHI)
 *
 * This is the ONLY interface the engine (tether_render.c, tether_layout.c)
 * is allowed to call. Concrete backends (Chitra, SDL, etc.) populate these
 * tables and drive the application loop.
 * ============================================================================ */

/* -------------------------------------------------------------------------- */
/* System API — lifecycle, frame control, clipping, resolution                */
/* -------------------------------------------------------------------------- */
typedef struct {
    void (*run)(Tether_App_Config* config);

    /* Physical window dimensions in pixels (w, h) */
    void (*get_size)(uint32_t* out_w, uint32_t* out_h);

    /* Resolution control (res_x, res_y. 0, 0 = matches window size) */
    void (*set_resolution)(uint32_t res_x, uint32_t res_y);
    void (*get_resolution)(uint32_t* out_x, uint32_t* out_y);

    /* Signals the backend that retained state has changed and must be rendered. */
    void (*mark_dirty)(void);

    /* Scissor clip region management */
    uint16_t (*register_clip)(int x, int y, int w, int h);
    void     (*update_clip)(uint32_t handle, int x, int y, int w, int h);
    uint16_t (*clip_intersect)(uint16_t parent_clip, int x, int y, int w, int h);
    uint16_t (*clip_intersect_to)(uint16_t existing_clip_id, uint16_t parent_clip, int x, int y, int w, int h);

    /* Element compositing properties */
    void     (*set_draw_order)(uint32_t handle, uint32_t draw_order);
    void     (*set_clip)(uint32_t handle, uint16_t clip_id);
    uint16_t (*get_clip_id)(uint32_t handle);
    void     (*release_clip)(uint32_t handle);

    bool (*is_alive)(uint32_t handle);
    void (*set_alive)(uint32_t handle, bool alive);
} Tether_System_API;

/* -------------------------------------------------------------------------- */
/* Shape API — retained SDF rounded rectangles, borders                       */
/* -------------------------------------------------------------------------- */
typedef struct {
    uint32_t (*create)(void);
    void     (*destroy)(uint32_t handle);
    bool     (*is_alive)(uint32_t handle);

    bool     (*set_bounds)(uint32_t handle, float x, float y, float w, float h);
    bool     (*set_radius)(uint32_t handle, float radius);
    bool     (*set_border)(uint32_t handle, float border_width);
    bool     (*set_color)(uint32_t handle, uint32_t fill_color, uint32_t border_color);
    bool     (*set_fill_color)(uint32_t handle, uint32_t fill_color);
    bool     (*set_border_color)(uint32_t handle, uint32_t border_color);
    bool     (*set_opacity)(uint32_t handle, float opacity);
    bool     (*set_rotation)(uint32_t handle, float origin_x, float origin_y, float angle_rad);
} Tether_Shape_API;

/* -------------------------------------------------------------------------- */
/* Text API — font loading, measuring, retained text element                  */
/* -------------------------------------------------------------------------- */
typedef struct {
    int      (*load_font)(const char* name, const char* path);
    void     (*unload_font)(int font_id);
    void     (*measure_text)(const char* text, uint32_t font_id, float size, float wrap_width, float* out_w, float* out_h);

    uint32_t (*create)(void);
    void     (*destroy)(uint32_t handle);
    bool     (*is_alive)(uint32_t handle);

    bool     (*set_text)(uint32_t handle, uint32_t font_id, float size, const char* text, float wrap_width, float x, float y);
    bool     (*set_color)(uint32_t handle, uint32_t color);
    bool     (*set_pos)(uint32_t handle, float x, float y);
    bool     (*set_opacity)(uint32_t handle, float opacity);
    bool     (*set_rotation)(uint32_t handle, float origin_x, float origin_y, float angle_rad);
    bool     (*set_scale)(uint32_t handle, float origin_x, float origin_y, float scale_x, float scale_y);
    bool     (*get_bounds)(uint32_t handle, float* out_w, float* out_h);
} Tether_Text_API;

/* Master Backend Interface */
typedef struct {
    Tether_System_API* system;
    Tether_Shape_API*  shape;
    Tether_Text_API*   text;
} Tether_Backend;

/* Typedef alias for backward compatibility */
typedef Tether_Backend Tether_RHI;

/* Global Backend instance */
extern Tether_Backend g_backend;
#define g_rhi g_backend

#endif /* TETHER_BACKEND_H */
