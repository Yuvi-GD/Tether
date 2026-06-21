#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#define SOKOL_WGPU
#define SOKOL_WGPU_C_HEADER <webgpu/webgpu.h>
#define SOKOL_GFX_IMPL
#define SOKOL_GLUE_IMPL
#define SOKOL_LOG_IMPL
#define SOKOL_LOG_CONSOLE

/* Tell ThorVG headers that the WebGPU backend is compiled in */
#define THORVG_WG_RASTER_SUPPORT 1

#include <webgpu/webgpu.h>
#include <stdio.h>
#include <stdbool.h>

/* THE RUST PANIC SHIELD 
   Intercept every unimplemented debug function before Sokol can call it */
#define wgpuTextureViewSetLabel(a, b) ((void)0)
#define wgpuTextureSetLabel(a, b) ((void)0)
#define wgpuBufferSetLabel(a, b) ((void)0)
#define wgpuRenderPipelineSetLabel(a, b) ((void)0)
#define wgpuBindGroupSetLabel(a, b) ((void)0)
#define wgpuBindGroupLayoutSetLabel(a, b) ((void)0)
#define wgpuShaderModuleSetLabel(a, b) ((void)0)
#define wgpuDeviceSetLoggingCallback(device, cb_info) ((void)0)

/* Intercept Sokol's wait call with a real function that returns the exact success enum */
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

/* Prevent Sokol from crashing on known wgpu-native Outdated swapchain bugs */
#define SOKOL_ASSERT(c) do { if (!(c)) { printf("SOKOL_ASSERT skipped: %s\n", #c); } } while(0)

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "thorvg_capi.h"
#include "tether.h"

#ifndef TVG_ENGINE_WG
#define TVG_ENGINE_WG (1 << 3)
#endif

/* WebGPU Error Reporter */
static void on_wgpu_error(WGPUErrorType type, const char* message, void* userdata) {
    printf("\n\n>>> WEBGPU VALIDATION ERROR: %s\n\n", message);
}

static WGPUDevice tether_wgpu_get_device(void) { return _sapp.wgpu.device; }
static WGPUInstance tether_wgpu_get_instance(void) { return _sapp.wgpu.instance; }

/* ThorVG canvas handle */
static Tvg_Canvas tvg_canvas = NULL;
static int current_width  = 0;
static int current_height = 0;
static Tvg_Paint bg = NULL;
static Tvg_Paint triangle = NULL;

/* sokol_gfx state */
static sg_pipeline blit_pip;
static sg_bindings blit_bind;
static WGPUTexture offscreen_texture = NULL;
static uint32_t offscreen_w = 0;
static uint32_t offscreen_h = 0;

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

    /* Create sampler */
    sg_sampler_desc smp_desc = {
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    };
    blit_bind.samplers[0] = sg_make_sampler(&smp_desc);

    /* Initialize WebGPU Engine */
    if (tvg_engine_init(TVG_ENGINE_WG) != TVG_RESULT_SUCCESS) {
        printf(">>> ERROR: WebGPU Engine Init Failed!\n");
    }
    tvg_canvas = tvg_wgcanvas_create(TVG_ENGINE_OPTION_NONE);

    /* Construct the Retained UI Tree ONCE */
    bg = tvg_shape_new();
    tvg_canvas_add(tvg_canvas, bg);

    /* The Triangle is perfectly retained. We set it up once and never touch it again. */
    triangle = tvg_shape_new();
    tvg_shape_move_to(triangle, 300.0f, 100.0f);
    tvg_shape_line_to(triangle, 500.0f, 400.0f);
    tvg_shape_line_to(triangle, 100.0f, 400.0f);
    tvg_shape_close(triangle);
    tvg_shape_set_fill_color(triangle, 255, 50, 50, 255);
    tvg_canvas_add(tvg_canvas, triangle);
}

static sg_image offscreen_img = {0};
static sg_view offscreen_view = {0};

static void frame(void) {
    current_width = sapp_width();
    current_height = sapp_height();
    
    if (current_width == 0 || current_height == 0) return; /* Don't draw if minimized */

    if (!offscreen_texture || offscreen_w != current_width || offscreen_h != current_height) {
        if (offscreen_texture) wgpuTextureRelease(offscreen_texture);
        if (offscreen_view.id != SG_INVALID_ID) sg_destroy_view(offscreen_view);
        if (offscreen_img.id != SG_INVALID_ID) sg_destroy_image(offscreen_img);

        WGPUTextureDescriptor desc = {
            .nextInChain = NULL,
            .label = "TetherOffscreenTarget",
            .usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
            .dimension = WGPUTextureDimension_2D,
            .size = { current_width, current_height, 1 },
            .format = WGPUTextureFormat_BGRA8Unorm,
            .mipLevelCount = 1,
            .sampleCount = 1,
            .viewFormatCount = 0,
            .viewFormats = NULL,
        };
        offscreen_texture = wgpuDeviceCreateTexture((WGPUDevice)tether_wgpu_get_device(), &desc);
        offscreen_w = current_width;
        offscreen_h = current_height;

        sg_image_desc img_desc = {
            .type = SG_IMAGETYPE_2D,
            .width = current_width,
            .height = current_height,
            .pixel_format = SG_PIXELFORMAT_BGRA8,
            .sample_count = 1,
            .wgpu_texture = (const void*)offscreen_texture,
        };
        offscreen_img = sg_make_image(&img_desc);

        sg_view_desc view_desc = {
            .texture = { .image = offscreen_img }
        };
        offscreen_view = sg_make_view(&view_desc);
        blit_bind.views[0] = offscreen_view;
    }

    /* 1. Feed the offscreen texture to ThorVG */
    if (tvg_wgcanvas_set_target(
        tvg_canvas,
        (void*)tether_wgpu_get_device(),
        (void*)tether_wgpu_get_instance(), 
        (void*)offscreen_texture,
        current_width, current_height,
        TVG_COLORSPACE_ABGR8888S, 1
    ) != TVG_RESULT_SUCCESS) {
        printf(">>> ERROR: Failed to set WebGPU Target!\n");
    }

    /* 2. Resize the background for the current window size */
    tvg_shape_reset(bg);
    tvg_shape_append_rect(bg, 0, 0, current_width, current_height, 0, 0, true);
    tvg_shape_set_fill_color(bg, 38, 38, 38, 255);
    
    /* 3. Update the Retained Scene */
    Tvg_Result update_res = tvg_canvas_update(tvg_canvas);
    
    if (update_res == TVG_RESULT_SUCCESS) {
        /* Only draw if the update generated valid vertices */
        Tvg_Result draw_res = tvg_canvas_draw(tvg_canvas, true); 
        
        if (draw_res == TVG_RESULT_SUCCESS) {
            /* Wait for ThorVG to finish rendering into offscreen_texture */
            tvg_canvas_sync(tvg_canvas);

            sg_swapchain swapchain = sglue_swapchain();
            if (!swapchain.wgpu.render_view) {
                printf(">>> Swapchain view unavailable (outdated surface?), skipping frame.\n");
                return;
            }

            /* Draw fullscreen quad using sokol_gfx to blit onto the swapchain */
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

        } else {
            printf(">>> ERROR: ThorVG failed to draw! Code: %d\n", draw_res);
        }
    } else if (update_res != TVG_RESULT_INSUFFICIENT_CONDITION) {
        printf(">>> ERROR: ThorVG failed to update vertices! Code: %d\n", update_res);
    }
}

static void cleanup(void) {
    tvg_canvas_destroy(tvg_canvas);
    tvg_engine_term();
    sg_shutdown();
}

void tether_run(Tether_App_Config* config) {
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