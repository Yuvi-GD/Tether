#ifndef TVG_ENGINE_WG
#define TVG_ENGINE_WG (1 << 3)
#endif

#include "thorvg_capi.h"
#include <webgpu/webgpu.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "tether/backends/tether_rhi.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_registry.h"

static Tvg_Canvas tvg_canvas = NULL;
static WGPUDevice cached_device = NULL;
static WGPUInstance cached_instance = NULL;
static WGPUTexture offscreen_texture = NULL;
static uint32_t offscreen_w = 0;
static uint32_t offscreen_h = 0;

static uint32_t rhi_max_w = 0;
static uint32_t rhi_max_h = 0;

void tether_rhi_init(uint32_t width, uint32_t height, uint32_t max_w, uint32_t max_h, const void* device, const void* instance) {
    cached_device = (WGPUDevice)device;
    cached_instance = (WGPUInstance)instance;
    rhi_max_w = max_w;
    rhi_max_h = max_h;

    /* Initialize WebGPU Engine */
    if (tvg_engine_init(TVG_ENGINE_WG) != TVG_RESULT_SUCCESS) {
        printf(">>> ERROR: WebGPU Engine Init Failed!\n");
    }
    
    tvg_canvas = tvg_wgcanvas_create(TVG_ENGINE_OPTION_NONE);

    /* Allocate initial texture */
    tether_rhi_resize(width, height);
}

void tether_rhi_load_font(const char* path) {
    if (!path) return;
    if (tvg_font_load(path) != TVG_RESULT_SUCCESS) {
        printf("[ThorVG RHI] ERROR: Failed to load font: %s\n", path);
    }
}

void tether_rhi_resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    
    /* Cap render resolution while maintaining aspect ratio */
    uint32_t render_w = width;
    uint32_t render_h = height;
    if (rhi_max_w > 0 && rhi_max_h > 0) {
        if (render_w > rhi_max_w || render_h > rhi_max_h) {
            float scale_x = (float)rhi_max_w / (float)render_w;
            float scale_y = (float)rhi_max_h / (float)render_h;
            float scale = scale_x < scale_y ? scale_x : scale_y;
            render_w = (uint32_t)(render_w * scale);
            render_h = (uint32_t)(render_h * scale);
            if (render_w == 0) render_w = 1;
            if (render_h == 0) render_h = 1;
        }
    }
    
    if (offscreen_texture && offscreen_w == render_w && offscreen_h == render_h) return;

    if (offscreen_texture) {
        wgpuTextureRelease(offscreen_texture);
    }

    WGPUTextureDescriptor desc = {
        .nextInChain = NULL,
        .label = "TetherOffscreenTarget",
        .usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
        .dimension = WGPUTextureDimension_2D,
        .size = { render_w, render_h, 1 },
        .format = WGPUTextureFormat_BGRA8Unorm,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .viewFormatCount = 0,
        .viewFormats = NULL,
    };
    offscreen_texture = wgpuDeviceCreateTexture(cached_device, &desc);
    offscreen_w = render_w;
    offscreen_h = render_h;
    
    /* Update ThorVG offscreen target */
    if (tvg_wgcanvas_set_target(
        tvg_canvas,
        (void*)cached_device,
        (void*)cached_instance, 
        (void*)offscreen_texture,
        render_w, render_h,
        TVG_COLORSPACE_ABGR8888S, 1
    ) != TVG_RESULT_SUCCESS) {
        printf(">>> ERROR: ThorVG failed to set WebGPU Target!\n");
    }
}

void tether_rhi_draw(void) {
    /* Update ThorVG scene graph */
    Tvg_Result update_res = tvg_canvas_update(tvg_canvas);
    
    if (update_res == TVG_RESULT_SUCCESS) {
        /* Only draw if the update generated valid vertices */
        Tvg_Result draw_res = tvg_canvas_draw(tvg_canvas, true); 
        
        if (draw_res == TVG_RESULT_SUCCESS) {
            /* Flush and submit ThorVG commands to the WebGPU queue.
             * (For ThorVG, sync is required to finalize the render task) */
            tvg_canvas_sync(tvg_canvas);
        } else {
            printf(">>> ERROR: ThorVG failed to draw! Code: %d\n", draw_res);
        }
    } else if (update_res != TVG_RESULT_INSUFFICIENT_CONDITION) {
        printf(">>> ERROR: ThorVG failed to update vertices! Code: %d\n", update_res);
    }
}

const void* tether_rhi_get_texture(void) {
    return (const void*)offscreen_texture;
}

void tether_rhi_get_texture_size(uint32_t* out_w, uint32_t* out_h) {
    if (out_w) *out_w = offscreen_w;
    if (out_h) *out_h = offscreen_h;
}

void tether_rhi_term(void) {
    /* 
     * NOTE: We deliberately skip tvg_canvas_destroy() and tvg_engine_term() 
     * here. In ThorVG's WebGPU backend, destroying the canvas or engine 
     * immediately upon window close can cause a thread deadlock/freeze 
     * if the WebGPU swapchain is in a specific state. 
     * Since this is only called at application shutdown, the OS will 
     * automatically reclaim the memory.
     */
    if (offscreen_texture) {
        wgpuTextureRelease(offscreen_texture);
    }
}

void tether_rhi_measure_text(const char* text, uint32_t font_id, int font_style, float font_size, float max_width, float* out_w, float* out_h) {
    if (!text || !out_w || !out_h) return;

    Tvg_Paint text_node = tvg_text_new();
    
    const char* font_name = tether_font_get_name(font_id);
    tvg_text_set_font(text_node, font_name ? font_name : "Roboto-Regular");
    
    tvg_text_set_size(text_node, font_size);
    tvg_text_set_text(text_node, text);

    /* If a max_width is provided, enable word wrapping before measuring */
    if (max_width > 0.0f) {
        tvg_text_layout(text_node, max_width, 0.0f);
        tvg_text_wrap_mode(text_node, TVG_TEXT_WRAP_WORD);
    }

    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    tvg_paint_get_aabb(text_node, &x, &y, &w, &h);
    
    *out_w = w;
    *out_h = h;

    tvg_paint_rel(text_node); /* Clean up the temporary paint node properly */

}
void tether_rhi_get_text_bounds(void* handle, float* tx, float* ty, float* w, float* h) {
    if (!handle) return;
    tvg_paint_get_aabb((Tvg_Paint)handle, tx, ty, w, h);
}

void tether_rhi_set_visible(void* handle, int visible) {
    if (!handle) return;
    tvg_paint_set_visible((Tvg_Paint)handle, visible ? true : false);
}

/* ============================================================================
 * RHI Abstraction Contract Implementations (Granular Setters)
 * ============================================================================ */

void* tether_rhi_create_rect(void) {
    Tvg_Paint shape = tvg_shape_new();
    tvg_paint_ref(shape);
    return (void*)shape;
}

void* tether_rhi_create_text(void) {
    Tvg_Paint text = tvg_text_new();
    tvg_paint_ref(text);
    return (void*)text;
}

void* tether_rhi_create_scene(void) {
    Tvg_Paint scene = tvg_scene_new();
    tvg_paint_ref(scene);
    return (void*)scene;
}

void tether_rhi_scene_push(void* scene_handle, void* child_handle) {
    if (!scene_handle || !child_handle) return;
    tvg_scene_add((Tvg_Paint)scene_handle, (Tvg_Paint)child_handle);
}

void tether_rhi_scene_remove(void* scene_handle, void* child_handle) {
    if (!scene_handle || !child_handle) return;
    tvg_scene_remove((Tvg_Paint)scene_handle, (Tvg_Paint)child_handle);
}

void tether_rhi_scene_clear(void* scene_handle) {
    if (!scene_handle) return;
    tvg_scene_remove((Tvg_Paint)scene_handle, NULL);
}

void tether_rhi_paint_free(void* handle) {
    if (!handle) return;
    tvg_paint_unref((Tvg_Paint)handle, true);
}

void tether_rhi_add_to_canvas(void* render_handle) {
    if (!render_handle) return;
    tvg_canvas_add(tvg_canvas, (Tvg_Paint)render_handle);
}

void tether_rhi_translate(void* render_handle, float x, float y) {
    if (!render_handle) return;
    tvg_paint_translate((Tvg_Paint)render_handle, x, y);
}

void tether_rhi_scale(void* render_handle, float factor_x, float factor_y) {
    if (!render_handle) return;
    tvg_paint_scale((Tvg_Paint)render_handle, factor_x); /* ThorVG C-API only supports uniform scale */
}

void tether_rhi_rotate(void* render_handle, float degrees) {
    if (!render_handle) return;
    tvg_paint_rotate((Tvg_Paint)render_handle, degrees);
}

void tether_rhi_set_opacity(void* render_handle, uint8_t alpha) {
    if (!render_handle) return;
    tvg_paint_set_opacity((Tvg_Paint)render_handle, alpha);
}

void tether_rhi_set_rect_geometry(void* render_handle, float w, float h, float rx, float ry) {
    if (!render_handle) return;
    Tvg_Paint paint = (Tvg_Paint)render_handle;
    tvg_shape_reset(paint);
    tvg_shape_append_rect(paint, 0, 0, w, h, rx, ry, true);
}

void tether_rhi_set_fill_color(void* render_handle, Tether_Color color) {
    if (!render_handle) return;
    tvg_shape_set_fill_color((Tvg_Paint)render_handle, color.r, color.g, color.b, color.a);
}

void tether_rhi_set_stroke_color(void* render_handle, Tether_Color color) {
    if (!render_handle) return;
    tvg_shape_set_stroke_color((Tvg_Paint)render_handle, color.r, color.g, color.b, color.a);
}

void tether_rhi_set_stroke_width(void* render_handle, float width) {
    if (!render_handle) return;
    tvg_shape_set_stroke_width((Tvg_Paint)render_handle, width);
}

void tether_rhi_set_clip_rect(void* render_handle, void* clip_rect_handle) {
    if (!render_handle) return;
    tvg_paint_set_clip((Tvg_Paint)render_handle, (Tvg_Paint)clip_rect_handle);
}

void tether_rhi_set_text_string(void* render_handle, const char* str) {
    if (!render_handle || !str) return;
    const char* current_str = tvg_text_get_text((Tvg_Paint)render_handle);
    if (current_str && strcmp(current_str, str) == 0) return;
    tvg_text_set_text((Tvg_Paint)render_handle, str);
}

void tether_rhi_set_text_font(void* render_handle, uint32_t font_id, int font_style, float font_size) {
    if (!render_handle) return;
    const char* font_name = tether_font_get_name(font_id);
    tvg_text_set_font((Tvg_Paint)render_handle, font_name ? font_name : "Roboto-Regular");
    tvg_text_set_size((Tvg_Paint)render_handle, font_size);
}

void tether_rhi_set_text_color(void* render_handle, Tether_Color color) {
    if (!render_handle) return;
    tvg_text_set_color((Tvg_Paint)render_handle, color.r, color.g, color.b);
}

void tether_rhi_set_text_wrap(void* render_handle, float max_width) {
    if (!render_handle) return;
    if (max_width > 0.0f) {
        tvg_text_layout((Tvg_Paint)render_handle, max_width, 0.0f);
        tvg_text_wrap_mode((Tvg_Paint)render_handle, TVG_TEXT_WRAP_WORD);
    } else {
        tvg_text_layout((Tvg_Paint)render_handle, 0.0f, 0.0f);
        tvg_text_wrap_mode((Tvg_Paint)render_handle, TVG_TEXT_WRAP_NONE);
    }
}