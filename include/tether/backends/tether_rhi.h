#ifndef TETHER_RHI_H
#define TETHER_RHI_H

#include <stdint.h>
#include "tether/core/tether_components.h"

/* V-Table API for dynamic RHI routing */
typedef struct {
    void (*init)(uint32_t width, uint32_t height, uint32_t max_w, uint32_t max_h, const void* device, const void* instance);
    void (*resize)(uint32_t width, uint32_t height);
    void (*term)(void);
    const void* (*get_texture)(void);
    void (*draw)(void);
    void (*load_font)(const char* path);
    void (*get_texture_size)(uint32_t* out_w, uint32_t* out_h);

    void* (*create_rect)(void);
    void* (*create_text)(void);
    void* (*create_scene)(void);

    void (*scene_push)(void* scene_handle, void* child_handle);
    void (*scene_remove)(void* scene_handle, void* child_handle);
    void (*scene_clear)(void* scene_handle);
    void (*add_to_canvas)(void* render_handle);
    void (*paint_free)(void* handle);

    void (*translate)(void* render_handle, float x, float y);
    void (*scale)(void* render_handle, float factor_x, float factor_y);
    void (*rotate)(void* render_handle, float degrees);
    void (*set_opacity)(void* handle, uint8_t opacity);
    void (*set_visible)(void* handle, int visible);
    void (*set_stroke_color)(void* render_handle, Tether_Color color);
    void (*set_stroke_width)(void* render_handle, float width);
    void (*set_clip_rect)(void* render_handle, void* clip_rect_handle);
    void (*get_text_bounds)(void* handle, float* tx, float* ty, float* w, float* h);

    void (*set_rect_geometry)(void* render_handle, float w, float h, float rx, float ry);
    void (*set_fill_color)(void* render_handle, Tether_Color color);
    void (*set_text_string)(void* render_handle, const char* str);
    void (*set_text_font)(void* render_handle, uint32_t font_id, int font_style, float font_size);
    void (*set_text_color)(void* render_handle, Tether_Color color);
    void (*set_text_wrap)(void* render_handle, float max_width);
    void (*measure_text)(const char* text, uint32_t font_id, int font_style, float font_size, float max_width, float* out_w, float* out_h);
} Tether_RHI_API;

/* Global RHI Interface exposed to the engine */
extern Tether_RHI_API* g_rhi;

#endif
