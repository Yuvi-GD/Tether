#ifndef TETHER_RHI_H
#define TETHER_RHI_H

#include <stdint.h>
#include <stdbool.h>
#include "tether/core/tether_components.h"

/* ============================================================================
 * Tether Rendering Hardware Interface (RHI)
 *
 * This is the ONLY interface the engine (tether_render.c, tether_layout.c)
 * is allowed to call. Concrete backends (Chitra, etc.) fill in the function
 * pointers and assign g_rhi before the engine starts.
 * ============================================================================ */

/* -------------------------------------------------------------------------- */
/* System API — lifecycle, frame control, clipping                            */
/* -------------------------------------------------------------------------- */
typedef struct {
    void (*init)(uint32_t width, uint32_t height, uint32_t max_w, uint32_t max_h, const void* device, const void* instance);
    void (*resize)(uint32_t width, uint32_t height);
    void (*term)(void);

    /* Returns the current render texture / swapchain size in pixels. */
    void (*get_texture_size)(uint32_t* out_w, uint32_t* out_h);

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

/* Master RHI Interface */
typedef struct {
    Tether_System_API* system;
    Tether_Shape_API*  shape;
    Tether_Text_API*   text;
} Tether_RHI;

/* Global RHI instance — set by the active backend before engine init. */
extern Tether_RHI g_rhi;

#endif /* TETHER_RHI_H */
