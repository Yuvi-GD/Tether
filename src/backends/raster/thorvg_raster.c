#ifndef TVG_ENGINE_WG
#define TVG_ENGINE_WG (1 << 3)
#endif

#include "thorvg_capi.h"
#include <webgpu/webgpu.h>
#include <stdio.h>
#include <stdbool.h>

#include "backends/tether_raster.h"

static Tvg_Canvas tvg_canvas = NULL;
static WGPUDevice cached_device = NULL;
static WGPUInstance cached_instance = NULL;
static WGPUTexture offscreen_texture = NULL;
static uint32_t offscreen_w = 0;
static uint32_t offscreen_h = 0;

static Tvg_Paint bg = NULL;
static Tvg_Paint triangle = NULL;

void tether_raster_init(uint32_t width, uint32_t height, const void* device, const void* instance) {
    cached_device = (WGPUDevice)device;
    cached_instance = (WGPUInstance)instance;

    /* Initialize WebGPU Engine */
    if (tvg_engine_init(TVG_ENGINE_WG) != TVG_RESULT_SUCCESS) {
        printf(">>> ERROR: WebGPU Engine Init Failed!\n");
    }
    tvg_canvas = tvg_wgcanvas_create(TVG_ENGINE_OPTION_NONE);

    /* Setup initial UI scene (Sandbox testing) */
    bg = tvg_shape_new();
    tvg_canvas_add(tvg_canvas, bg);

    triangle = tvg_shape_new();
    tvg_shape_move_to(triangle, 300.0f, 100.0f);
    tvg_shape_line_to(triangle, 500.0f, 400.0f);
    tvg_shape_line_to(triangle, 100.0f, 400.0f);
    tvg_shape_close(triangle);
    tvg_shape_set_fill_color(triangle, 255, 50, 50, 255);
    tvg_canvas_add(tvg_canvas, triangle);

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

    /* Resize background rect */
    tvg_shape_reset(bg);
    tvg_shape_append_rect(bg, 0, 0, width, height, 0, 0, true);
    tvg_shape_set_fill_color(bg, 38, 38, 38, 255);
}

void tether_raster_draw(void) {
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
