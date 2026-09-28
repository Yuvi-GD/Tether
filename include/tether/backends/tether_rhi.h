#ifndef TETHER_RHI_H
#define TETHER_RHI_H

#include <stdint.h>
#include "tether/core/tether_components.h"

/* System API (Init/Term/Core loop) */
typedef struct {
    void (*init)(uint32_t width, uint32_t height, uint32_t max_w, uint32_t max_h, const void* device, const void* instance);
    void (*resize)(uint32_t width, uint32_t height);
    void (*term)(void);
    const void* (*get_texture)(void);
    void (*get_texture_size)(uint32_t* out_w, uint32_t* out_h);
    void (*draw)(void);
    void (*draw_fullscreen_texture)(const void* texture);
} Tether_System_API;

/* Shape API (Handled by Sokol) */
typedef struct {
    void* (*create_rect)(void);
    void (*set_rect_geometry)(void* render_handle, float w, float h, float rx, float ry);
    void (*set_fill_color)(void* render_handle, Tether_Color color);
    void (*set_visible)(void* handle, int visible);
    void (*set_opacity)(void* handle, uint8_t opacity);
    void (*translate)(void* render_handle, float x, float y);
    void (*scale)(void* render_handle, float factor_x, float factor_y);
    void (*rotate)(void* render_handle, float degrees);
    void (*set_z_index)(void* render_handle, int z_index);
    void (*destroy)(void* render_handle);
} Tether_Shape_API;

/* Text API (Temporarily ThorVG) */
typedef struct {
    void (*load_font)(const char* path);
    void* (*create_text)(void);
    void (*set_text_string)(void* render_handle, const char* str);
    void (*set_text_font)(void* render_handle, uint32_t font_id, int font_style, float font_size);
    void (*set_text_color)(void* render_handle, Tether_Color color);
    void (*set_text_wrap)(void* render_handle, float max_width);
    void (*set_visible)(void* handle, int visible);
    void (*set_opacity)(void* handle, uint8_t opacity);
    void (*translate)(void* render_handle, float x, float y);
    void (*scale)(void* render_handle, float factor_x, float factor_y);
    void (*rotate)(void* render_handle, float degrees);
    void (*set_z_index)(void* render_handle, int z_index);
    void (*get_text_bounds)(void* handle, float* tx, float* ty, float* w, float* h);
    void (*measure_text)(const char* text, uint32_t font_id, int font_style, float font_size, float max_width, float* out_w, float* out_h);
    void (*destroy)(void* render_handle);
} Tether_Text_API;

/* Master RHI Interface */
typedef struct {
    Tether_System_API* system;
    Tether_Shape_API* shape;
    Tether_Text_API* text;
} Tether_RHI;

/* Global RHI Instance exposed to the engine */
extern Tether_RHI g_rhi;

#endif
