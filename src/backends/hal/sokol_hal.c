#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#define SOKOL_WGPU
#define SOKOL_WGPU_C_HEADER <webgpu/webgpu.h>
#define SOKOL_GFX_IMPL
#define SOKOL_GLUE_IMPL
#define SOKOL_LOG_IMPL
#define SOKOL_LOG_CONSOLE

#include <webgpu/webgpu.h>
#include <stdio.h>
#include <stdbool.h>

#include "tether.h"
#include "backends/tether_hal.h"
#include "backends/tether_raster.h"

/* Required for macOS (Metal integration) */
#define wgpuTextureViewSetLabel(a, b) ((void)0)
#define wgpuTextureSetLabel(a, b) ((void)0)
#define wgpuBufferSetLabel(a, b) ((void)0)
#define wgpuRenderPipelineSetLabel(a, b) ((void)0)
#define wgpuBindGroupSetLabel(a, b) ((void)0)
#define wgpuBindGroupLayoutSetLabel(a, b) ((void)0)
#define wgpuShaderModuleSetLabel(a, b) ((void)0)
#define wgpuDeviceSetLoggingCallback(device, cb_info) ((void)0)

typedef enum WGPULoggingType {
    WGPULoggingType_Verbose = 0x00000001,
    WGPULoggingType_Info = 0x00000002,
    WGPULoggingType_Warning = 0x00000003,
    WGPULoggingType_Error = 0x00000004,
    WGPULoggingType_Force32 = 0x7FFFFFFF
} WGPULoggingType;

typedef struct WGPULoggingCallbackInfo {
    void * callback;
    void*  userdata1;
    void*  userdata2;
} WGPULoggingCallbackInfo;

static WGPUWaitStatus tether_dummy_wait(WGPUInstance instance, size_t futureCount, WGPUFutureWaitInfo* futures, uint64_t timeoutNS) {
    return WGPUWaitStatus_Success;
}
#define wgpuInstanceWaitAny tether_dummy_wait

static bool global_swapchain_view_acquired = false;

#define wgpuSurfaceGetCurrentTexture(a, b) tether_wgpuSurfaceGetCurrentTexture(a, b)
static inline void tether_wgpuSurfaceGetCurrentTexture(WGPUSurface surface, WGPUSurfaceTexture* surfaceTexture) {
    (wgpuSurfaceGetCurrentTexture)(surface, surfaceTexture);
    if (surfaceTexture->status == 0 || surfaceTexture->status == 1) { /* Optimal or Suboptimal */
        global_swapchain_view_acquired = true;
    } else {
        global_swapchain_view_acquired = false;
    }
}

#define wgpuSurfacePresent(a) tether_wgpuSurfacePresent(a)
static inline void tether_wgpuSurfacePresent(WGPUSurface surface) {
    if (global_swapchain_view_acquired) {
        (wgpuSurfacePresent)(surface);
        global_swapchain_view_acquired = false;
    }
}

/* Prevent Sokol from crashing on known wgpu-native Outdated swapchain issues.
   Supressing printf to avoid thread blocking during continuous resize events. */
#define SOKOL_ASSERT(c) ((void)0)

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"

#if defined(_WIN32)
#include <windows.h>
#include <dwmapi.h>
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

/* sokol_gfx state */
static sg_pipeline blit_pip;
static sg_bindings blit_bind;
static sg_image offscreen_img = {0};
static sg_view offscreen_view = {0};
static WGPUTexture current_wgpu_texture = NULL;

static void init(void) {
    /* Initialize sokol_gfx */
    sg_desc sgdesc = {
        .environment = sglue_environment(),
        .logger.func = slog_func
    };
    sg_setup(&sgdesc);

    /* Setup fullscreen quad shader & pipeline */
    sg_shader_desc shd_desc = {
        .vertex_func.source = 
            "struct vs_out {\n"
            "  @builtin(position) pos: vec4<f32>,\n"
            "  @location(0) uv: vec2<f32>,\n"
            "};\n"
            "@vertex fn main(@builtin(vertex_index) vid: u32) -> vs_out {\n"
            "  var pos = array<vec2<f32>, 3>(vec2<f32>(-1.0, -1.0), vec2<f32>(3.0, -1.0), vec2<f32>(-1.0, 3.0));\n"
            "  var uv = array<vec2<f32>, 3>(vec2<f32>(0.0, 1.0), vec2<f32>(2.0, 1.0), vec2<f32>(0.0, -1.0));\n"
            "  var out: vs_out;\n"
            "  out.pos = vec4<f32>(pos[vid], 0.0, 1.0);\n"
            "  out.uv = uv[vid];\n"
            "  return out;\n"
            "}\n",
        .fragment_func.source = 
            "@group(1) @binding(0) var tex: texture_2d<f32>;\n"
            "@group(1) @binding(1) var smp: sampler;\n"
            "@fragment fn main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {\n"
            "  return textureSample(tex, smp, uv);\n"
            "}\n",
        .views[0].texture = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .wgsl_group1_binding_n = 0,
            .image_type = SG_IMAGETYPE_2D,
            .sample_type = SG_IMAGESAMPLETYPE_FLOAT,
        },
        .samplers[0] = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .wgsl_group1_binding_n = 1,
            .sampler_type = SG_SAMPLERTYPE_FILTERING,
        },
        .texture_sampler_pairs[0] = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .view_slot = 0,
            .sampler_slot = 0,
            .glsl_name = "tex_smp", /* required by sokol validation */
        }
    };
    sg_shader shd = sg_make_shader(&shd_desc);

    sg_pipeline_desc pip_desc = {
        .shader = shd,
        .layout = {
            .attrs[0].format = SG_VERTEXFORMAT_INVALID
        },
        .depth = {
            .compare = SG_COMPAREFUNC_ALWAYS,
            .write_enabled = false
        },
    };
    blit_pip = sg_make_pipeline(&pip_desc);

    /* Create linear sampler */
    sg_sampler_desc smp_desc = {
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    };
    blit_bind.samplers[0] = sg_make_sampler(&smp_desc);

#if defined(_WIN32)
    /* Enable Windows 10/11 Dark Mode Title Bar */
    HWND hwnd = (HWND)sapp_win32_get_hwnd();
    if (hwnd) {
        BOOL value = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    }
#endif

    /* Initialize Rasterizer with Sokol WebGPU Device & Instance */
    tether_raster_init(sapp_width(), sapp_height(), _sapp.wgpu.device, _sapp.wgpu.instance);
}

static void frame(void) {
    int w = sapp_width();
    int h = sapp_height();
    if (w == 0 || h == 0) return;

    /* Skip rendering if swapchain is invalid (e.g. during Win32 modal resize loops). */
    sg_swapchain swapchain = sglue_swapchain();
    if (!swapchain.wgpu.render_view) {
        return;
    }

    tether_raster_resize(w, h);
    tether_raster_draw();

    WGPUTexture raster_tex = (WGPUTexture)tether_raster_get_texture();
    if (!raster_tex) return;

    /* Update sokol_gfx bindings if rasterizer created a new texture */
    if (raster_tex != current_wgpu_texture) {
        if (offscreen_view.id != SG_INVALID_ID) sg_destroy_view(offscreen_view);
        if (offscreen_img.id != SG_INVALID_ID) sg_destroy_image(offscreen_img);

        sg_image_desc img_desc = {
            .type = SG_IMAGETYPE_2D,
            .width = w,
            .height = h,
            .pixel_format = SG_PIXELFORMAT_BGRA8,
            .sample_count = 1,
            .wgpu_texture = (const void*)raster_tex,
        };
        offscreen_img = sg_make_image(&img_desc);

        sg_view_desc view_desc = {
            .texture = { .image = offscreen_img }
        };
        offscreen_view = sg_make_view(&view_desc);
        blit_bind.views[0] = offscreen_view;
        
        current_wgpu_texture = raster_tex;
    }

    /* Fullscreen blit */
    sg_pass pass = {
        .action = {
            .colors[0] = { .load_action = SG_LOADACTION_DONTCARE }
        },
        .swapchain = swapchain
    };
    sg_begin_pass(&pass);
    sg_apply_pipeline(blit_pip);
    sg_apply_bindings(&blit_bind);
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
}

static void cleanup(void) {
    tether_raster_term();
    sg_shutdown();
}

void tether_hal_run(Tether_App_Config* config) {
    sapp_desc desc      = {0};
    desc.init_cb        = init;
    desc.frame_cb       = frame;
    desc.cleanup_cb     = cleanup;
    desc.width          = config->width;
    desc.height         = config->height;
    desc.window_title   = config->title;
    desc.logger.func    = slog_func;

    sapp_run(&desc);
}
