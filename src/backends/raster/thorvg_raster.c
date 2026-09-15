#ifndef TVG_ENGINE_WG
#define TVG_ENGINE_WG (1 << 3)
#endif

#include "thorvg_capi.h"
#include <webgpu/webgpu.h>
#include <stdio.h>
#include <stdbool.h>

#include "backends/tether_raster.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"

static Tvg_Canvas tvg_canvas = NULL;
static WGPUDevice cached_device = NULL;
static WGPUInstance cached_instance = NULL;
static WGPUTexture offscreen_texture = NULL;
static uint32_t offscreen_w = 0;
static uint32_t offscreen_h = 0;

void tether_raster_init(uint32_t width, uint32_t height, const void* device, const void* instance) {
    cached_device = (WGPUDevice)device;
    cached_instance = (WGPUInstance)instance;

    /* Initialize WebGPU Engine */
    if (tvg_engine_init(TVG_ENGINE_WG) != TVG_RESULT_SUCCESS) {
        printf(">>> ERROR: WebGPU Engine Init Failed!\n");
    }
    
    /* Load fonts from the registry */
    for (uint32_t i = 1; i <= 31; ++i) {
        const char* path = tether_font_get_path(i);
        if (path) {
            if (tvg_font_load(path) != TVG_RESULT_SUCCESS) {
                printf(">>> ERROR: Failed to load font: %s\n", path);
            }
        }
    }

    tvg_canvas = tvg_wgcanvas_create(TVG_ENGINE_OPTION_NONE);

    /* Allocate initial texture */
    tether_raster_resize(width, height);
}

void tether_raster_resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    if (offscreen_texture && offscreen_w == width && offscreen_h == height) return;

    if (offscreen_texture) {
        wgpuTextureRelease(offscreen_texture);
    }

    WGPUTextureDescriptor desc = {
        .nextInChain = NULL,
        .label = "TetherOffscreenTarget",
        .usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
        .dimension = WGPUTextureDimension_2D,
        .size = { width, height, 1 },
        .format = WGPUTextureFormat_BGRA8Unorm,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .viewFormatCount = 0,
        .viewFormats = NULL,
    };
    offscreen_texture = wgpuDeviceCreateTexture(cached_device, &desc);
    offscreen_w = width;
    offscreen_h = height;
    
    /* Update ThorVG offscreen target */
    if (tvg_wgcanvas_set_target(
        tvg_canvas,
        (void*)cached_device,
        (void*)cached_instance, 
        (void*)offscreen_texture,
        width, height,
        TVG_COLORSPACE_ABGR8888S, 1
    ) != TVG_RESULT_SUCCESS) {
        printf(">>> ERROR: ThorVG failed to set WebGPU Target!\n");
    }
}

void tether_raster_draw(void) {
    /* Clear previous frame's geometry and free memory */
    tvg_canvas_remove(tvg_canvas, NULL);
    
    /* Draw Background */
    Tvg_Paint bg_rect = tvg_shape_new();
    tvg_shape_append_rect(bg_rect, 0, 0, (float)offscreen_w, (float)offscreen_h, 0, 0, true);
    tvg_shape_set_fill_color(bg_rect, 38, 38, 38, 255);
    tvg_canvas_add(tvg_canvas, bg_rect);
    
    /* Draw ECS Scene */
    Tether_DenseArray* transforms = tether_ecs_get_dense_array(TETHER_COMPONENT_SLOT_TRANSFORM);
    if (transforms) {
        for (uint32_t i = 0; i < transforms->count; i++) {
            Tether_GUID entity = transforms->entity_map[i];
            if (!tether_ecs_is_valid(entity)) continue;

            Tether_SlotTransform* t = (Tether_SlotTransform*)((uint8_t*)transforms->data + (i * transforms->element_size));
            Tether_Color* c = (Tether_Color*)tether_ecs_get_component(entity, TETHER_COMPONENT_COLOR);
            Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_get_component(entity, TETHER_COMPONENT_RENDER_TRANSFORM);
            Tether_TextStyle* text = (Tether_TextStyle*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_STYLE);

            /* Render Background (Only if NOT a Text node, or if we introduce a separate bg_color later) */
            if (c && !text) {
                Tvg_Paint shape = tvg_shape_new();
                tvg_shape_append_rect(shape, t->x, t->y, t->width, t->height, 0, 0, true);
                tvg_shape_set_fill_color(shape, c->r, c->g, c->b, c->a);
                
                /* Check for Render Transform (Animations / Visual Offsets) */
                Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_get_component(entity, TETHER_COMPONENT_RENDER_TRANSFORM);
                if (rt) {
                    /* Pivot calculation could be done manually, but for now we apply scale, rotation and translation */
                    tvg_paint_translate(shape, rt->translation_x, rt->translation_y);
                    if (rt->scale_x != 0.0f || rt->scale_y != 0.0f) {
                        /* To fully support Pivot, we would translate to pivot, scale, then translate back. 
                           For this initial pass, we just apply standard scale. */
                        tvg_paint_scale(shape, rt->scale_x);
                    }
                    if (rt->rotation_deg != 0.0f) {
                        tvg_paint_rotate(shape, rt->rotation_deg);
                    }
                }
                
                tvg_canvas_add(tvg_canvas, shape);
            }

            /* Render Text */
            if (text) {
                const char* str = tether_ecs_get_text_string(entity);
                if (str && str[0] != '\0') {
                    Tvg_Paint text_node = tvg_text_new();
                    
                    const char* font_name = tether_font_get_name(text->font_id);
                    tvg_text_set_font(text_node, font_name ? font_name : "Roboto-Regular");
                    
                    tvg_text_set_size(text_node, text->font_size);
                    tvg_text_set_text(text_node, str);
                    
                    if (c) {
                        tvg_text_set_color(text_node, c->r, c->g, c->b);
                        tvg_paint_set_opacity(text_node, c->a);
                    } else {
                        tvg_text_set_color(text_node, 255, 255, 255);
                        tvg_paint_set_opacity(text_node, 255);
                    }
                    
                    /* Enable Text Wrapping to bounds of the text slot */
                    tvg_text_layout(text_node, t->width, t->height);
                    tvg_text_wrap_mode(text_node, TVG_TEXT_WRAP_WORD);
                    
                    /* Simple alignment logic based on intrinsic bounds */
                    float tx = t->x;
                    float ty = t->y;
                    
                    float t_x, t_y, tw, th;
                    tvg_paint_get_aabb(text_node, &t_x, &t_y, &tw, &th);
                    
                    if (text->align_x == TETHER_ALIGN_CENTER) {
                        tx += (t->width - tw) * 0.5f;
                    } else if (text->align_x == TETHER_ALIGN_RIGHT) {
                        tx += (t->width - tw);
                    }
                    
                    if (text->align_y == TETHER_ALIGN_CENTER) {
                        ty += (t->height - th) * 0.5f;
                    } else if (text->align_y == TETHER_ALIGN_BOTTOM) {
                        ty += (t->height - th);
                    }

                    /* Compensate for ThorVG's internal origin of the text bounding box */
                    tx -= t_x;
                    ty -= t_y;

                    tvg_paint_translate(text_node, tx, ty);
                    
                    Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_get_component(entity, TETHER_COMPONENT_RENDER_TRANSFORM);
                    if (rt) {
                        tvg_paint_translate(text_node, tx + rt->translation_x, ty + rt->translation_y);
                        if (rt->scale_x != 0.0f || rt->scale_y != 0.0f) {
                            tvg_paint_scale(text_node, rt->scale_x);
                        }
                        if (rt->rotation_deg != 0.0f) {
                            tvg_paint_rotate(text_node, rt->rotation_deg);
                        }
                    }

                    tvg_canvas_add(tvg_canvas, text_node);
                }
            }
        }
    }

    /* Update ThorVG scene graph */
    Tvg_Result update_res = tvg_canvas_update(tvg_canvas);
    
    if (update_res == TVG_RESULT_SUCCESS) {
        /* Only draw if the update generated valid vertices */
        Tvg_Result draw_res = tvg_canvas_draw(tvg_canvas, true); 
        
        if (draw_res == TVG_RESULT_SUCCESS) {
            /* Wait for ThorVG to finish rendering into offscreen_texture */
            tvg_canvas_sync(tvg_canvas);
        } else {
            printf(">>> ERROR: ThorVG failed to draw! Code: %d\n", draw_res);
        }
    } else if (update_res != TVG_RESULT_INSUFFICIENT_CONDITION) {
        printf(">>> ERROR: ThorVG failed to update vertices! Code: %d\n", update_res);
    }
}

const void* tether_raster_get_texture(void) {
    return (const void*)offscreen_texture;
}

void tether_raster_term(void) {
    if (tvg_canvas) {
        tvg_canvas_destroy(tvg_canvas);
    }
    tvg_engine_term();
    if (offscreen_texture) {
        wgpuTextureRelease(offscreen_texture);
    }
}

void tether_raster_measure_text(const char* text, uint32_t font_id, int font_style, float font_size, float* out_w, float* out_h) {
    if (!text || !out_w || !out_h) return;

    Tvg_Paint text_node = tvg_text_new();
    
    const char* font_name = tether_font_get_name(font_id);
    tvg_text_set_font(text_node, font_name ? font_name : "Roboto-Regular");
    
    tvg_text_set_size(text_node, font_size);
    tvg_text_set_text(text_node, text);

    float w = 0.0f, h = 0.0f;
    tvg_paint_get_aabb(text_node, NULL, NULL, &w, &h);
    
    *out_w = w;
    *out_h = h;

    tvg_paint_rel(text_node); /* Clean up the temporary paint node */
}
