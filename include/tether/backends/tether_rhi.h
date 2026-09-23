#ifndef TETHER_RHI_H
#define TETHER_RHI_H

#include <stdint.h>
#include "tether/core/tether_components.h"

/* Lifecycle (Called by HAL) */
void tether_rhi_init(uint32_t width, uint32_t height, const void* device, const void* instance);
void tether_rhi_resize(uint32_t width, uint32_t height);
void tether_rhi_term(void);
const void* tether_rhi_get_texture(void);
void  tether_rhi_draw(void);

/* Granular Property Sync API */
void* tether_rhi_create_rect(void);
void* tether_rhi_create_text(void);
void* tether_rhi_create_scene(void);

void tether_rhi_scene_push(void* scene_handle, void* child_handle);
void tether_rhi_scene_remove(void* scene_handle, void* child_handle);
void tether_rhi_scene_clear(void* scene_handle);
void tether_rhi_add_to_canvas(void* render_handle);
void tether_rhi_paint_free(void* handle);

/* Common Transforms (Works on any node: Rect, Text, etc.) */
void tether_rhi_translate(void* render_handle, float x, float y);
void tether_rhi_scale(void* render_handle, float factor_x, float factor_y);
void tether_rhi_rotate(void* render_handle, float degrees);
void tether_rhi_set_opacity(void* handle, uint8_t opacity);
void tether_rhi_set_visible(void* handle, int visible);
void tether_rhi_get_text_bounds(void* handle, float* tx, float* ty, float* w, float* h);

/* Specific Geometry Setters */
void tether_rhi_set_rect_geometry(void* render_handle, float w, float h, float rx, float ry);
void tether_rhi_set_fill_color(void* render_handle, Tether_Color color);
void tether_rhi_set_text_string(void* render_handle, const char* str);
void tether_rhi_set_text_font(void* render_handle, uint32_t font_id, int font_style, float font_size);
void tether_rhi_set_text_color(void* render_handle, Tether_Color color);
void tether_rhi_set_text_wrap(void* render_handle, float max_width);
void tether_rhi_measure_text(const char* text, uint32_t font_id, int font_style, float font_size, float max_width, float* out_w, float* out_h);

#endif
