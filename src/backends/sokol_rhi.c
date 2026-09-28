#include "tether/backends/tether_rhi.h"
#include <sokol_gfx.h>
#include <sokol_app.h>
#include <stdlib.h>
#include <string.h>
#include <webgpu/webgpu.h>

#define MAX_VERTICES 65536
#define MAX_INDICES 65536
#define MAX_NODES 4096

typedef struct {
    float pos[2];
    float uv[2];
    float color[4];
    float size[2];
    float radius;
} Vertex;

typedef struct {
    float x, y;
    float w, h;
    float rx, ry;
    Tether_Color color;
    bool is_visible;
    bool is_alive;
    int z_index;
} RenderNode;

static struct {
    sg_pipeline pip;
    sg_bindings bind;
    
    Vertex vertices[MAX_VERTICES];
    uint16_t indices[MAX_INDICES];
    int v_count;
    int i_count;
    
    RenderNode nodes[MAX_NODES];
    int node_count;
    
    sg_image offscreen_img;
    sg_view offscreen_view;
    sg_pass_action pass_action;
    
    sg_pipeline blit_pip;
    sg_bindings blit_bind;
    sg_sampler smp;
    sg_image custom_img;
    sg_view custom_view;
    const void* last_custom_tex;
    
    uint32_t width, height;
} s_rhi = {0};

#include "ui_rect.glsl.h"

static void sokol_rhi_resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    if (s_rhi.width == width && s_rhi.height == height) return;

    s_rhi.width = width;
    s_rhi.height = height;

    if (s_rhi.offscreen_img.id != SG_INVALID_ID) {
        sg_destroy_image(s_rhi.offscreen_img);
    }

    sg_image_desc img_desc = {
        .usage = { .color_attachment = true },
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_BGRA8,
        .sample_count = 1,
        .label = "sokol_rhi_offscreen"
    };
    s_rhi.offscreen_img = sg_make_image(&img_desc);
    
    sg_view_desc view_desc = {
        .color_attachment = { .image = s_rhi.offscreen_img }
    };
    s_rhi.offscreen_view = sg_make_view(&view_desc);
}

static void sokol_rhi_init(uint32_t width, uint32_t height, uint32_t max_w, uint32_t max_h, const void* device, const void* instance) {
    /* Create Buffers */
    sg_buffer_desc vbuf_desc = {
        .usage = { .vertex_buffer = true, .dynamic_update = true },
        .size = sizeof(s_rhi.vertices),
        .label = "ui_vertices"
    };
    s_rhi.bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    sg_buffer_desc ibuf_desc = {
        .usage = { .index_buffer = true, .dynamic_update = true },
        .size = sizeof(s_rhi.indices),
        .label = "ui_indices"
    };
    s_rhi.bind.index_buffer = sg_make_buffer(&ibuf_desc);

    /* Create Shader using sokol-shdc generated desc */
    sg_shader shd = sg_make_shader(ui_rect_shader_desc(sg_query_backend()));
    
    /* Blit Pipeline Setup */
    static float blit_verts[] = { -1,1,0,0,  -1,-1,0,1,  1,1,1,0,  1,-1,1,1 };
    sg_buffer_desc bv_desc = { .data = SG_RANGE(blit_verts) };
    s_rhi.blit_bind.vertex_buffers[0] = sg_make_buffer(&bv_desc);
    
    sg_sampler_desc smp_desc = { .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR };
    s_rhi.smp = sg_make_sampler(&smp_desc);
    s_rhi.blit_bind.samplers[0] = s_rhi.smp;

    sg_pipeline_desc bp_desc = {
        .shader = sg_make_shader(ui_blit_shader_desc(sg_query_backend())),
        .layout = { .attrs = { [0] = {.format=SG_VERTEXFORMAT_FLOAT2}, [1] = {.format=SG_VERTEXFORMAT_FLOAT2} } },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .colors[0] = {
            .pixel_format = SG_PIXELFORMAT_BGRA8,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_ONE,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
            }
        },
        .depth = { .pixel_format = SG_PIXELFORMAT_NONE },
        .label = "blit_pip"
    };
    s_rhi.blit_pip = sg_make_pipeline(&bp_desc);
    


    /* Create Pipeline */
    sg_pipeline_desc pip_desc = {
        .shader = shd,
        .layout = {
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT2 }, // pos
                [1] = { .format = SG_VERTEXFORMAT_FLOAT2 }, // uv
                [2] = { .format = SG_VERTEXFORMAT_FLOAT4 }, // color
                [3] = { .format = SG_VERTEXFORMAT_FLOAT2 }, // size
                [4] = { .format = SG_VERTEXFORMAT_FLOAT },  // radius
            }
        },
        .colors[0] = {
            .pixel_format = SG_PIXELFORMAT_BGRA8,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_ONE,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .cull_mode = SG_CULLMODE_NONE,
        .depth = { .pixel_format = SG_PIXELFORMAT_NONE, .write_enabled = false },
        .label = "ui_pipeline"
    };
    s_rhi.pip = sg_make_pipeline(&pip_desc);
    
    s_rhi.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = {0.0f, 0.0f, 0.0f, 0.0f} }
    };

    sokol_rhi_resize(width, height);
}

static void sokol_rhi_term(void) {
    if (s_rhi.offscreen_img.id != SG_INVALID_ID) sg_destroy_image(s_rhi.offscreen_img);
    if (s_rhi.custom_img.id != SG_INVALID_ID) sg_destroy_image(s_rhi.custom_img);
}

static const void* sokol_rhi_get_texture(void) {
    if (s_rhi.offscreen_img.id == SG_INVALID_ID) return NULL;
    return (const void*)(intptr_t)s_rhi.offscreen_img.id;
}

static void sokol_rhi_get_texture_size(uint32_t* out_w, uint32_t* out_h) {
    if (out_w) *out_w = s_rhi.width;
    if (out_h) *out_h = s_rhi.height;
}

/* Helper for qsort */
static int compare_z_index(const void* a, const void* b) {
    RenderNode* node_a = *(RenderNode**)a;
    RenderNode* node_b = *(RenderNode**)b;
    return node_a->z_index - node_b->z_index;
}

static void sokol_rhi_draw(void) {    
    s_rhi.v_count = 0;
    s_rhi.i_count = 0;

    /* Collect and sort alive nodes */
    RenderNode* sorted_nodes[MAX_NODES];
    int sort_count = 0;
    for (int i = 0; i < s_rhi.node_count; ++i) {
        if (s_rhi.nodes[i].is_alive && s_rhi.nodes[i].is_visible && s_rhi.nodes[i].color.a > 0) {
            sorted_nodes[sort_count++] = &s_rhi.nodes[i];
        }
    }
    
    if (sort_count > 0) {
        qsort(sorted_nodes, sort_count, sizeof(RenderNode*), compare_z_index);
    }

    /* Build Batch */
    for (int i = 0; i < sort_count; ++i) {
        RenderNode* n = sorted_nodes[i];

        if (s_rhi.v_count + 4 > MAX_VERTICES || s_rhi.i_count + 6 > MAX_INDICES) break;

        uint16_t v_idx = s_rhi.v_count;
        
        float c[4] = { n->color.r / 255.0f, n->color.g / 255.0f, n->color.b / 255.0f, n->color.a / 255.0f };

        s_rhi.vertices[v_idx+0] = (Vertex){{n->x, n->y}, {0,0}, {c[0],c[1],c[2],c[3]}, {n->w,n->h}, n->rx};
        s_rhi.vertices[v_idx+1] = (Vertex){{n->x+n->w, n->y}, {1,0}, {c[0],c[1],c[2],c[3]}, {n->w,n->h}, n->rx};
        s_rhi.vertices[v_idx+2] = (Vertex){{n->x+n->w, n->y+n->h}, {1,1}, {c[0],c[1],c[2],c[3]}, {n->w,n->h}, n->rx};
        s_rhi.vertices[v_idx+3] = (Vertex){{n->x, n->y+n->h}, {0,1}, {c[0],c[1],c[2],c[3]}, {n->w,n->h}, n->rx};

        s_rhi.indices[s_rhi.i_count+0] = v_idx + 0;
        s_rhi.indices[s_rhi.i_count+1] = v_idx + 1;
        s_rhi.indices[s_rhi.i_count+2] = v_idx + 2;
        s_rhi.indices[s_rhi.i_count+3] = v_idx + 0;
        s_rhi.indices[s_rhi.i_count+4] = v_idx + 2;
        s_rhi.indices[s_rhi.i_count+5] = v_idx + 3;

        s_rhi.v_count += 4;
        s_rhi.i_count += 6;
    }

    if (s_rhi.v_count > 0) {
        sg_update_buffer(s_rhi.bind.vertex_buffers[0], &SG_RANGE(s_rhi.vertices));
        sg_update_buffer(s_rhi.bind.index_buffer, &SG_RANGE(s_rhi.indices));
    }

    /* Render Pass */
    sg_pass pass = {
        .action = s_rhi.pass_action,
        .attachments = {
            .colors[0] = s_rhi.offscreen_view
        }
    };
    sg_begin_pass(&pass);
    
    /* Draw Native UI Shapes */
    if (s_rhi.i_count > 0) {
        sg_apply_pipeline(s_rhi.pip);
        sg_apply_bindings(&s_rhi.bind);
        float res[4] = { (float)s_rhi.width, (float)s_rhi.height, 0.0f, 0.0f };
        sg_apply_uniforms(0, &SG_RANGE(res));
        sg_draw(0, s_rhi.i_count, 1);
    }
    

    sg_end_pass();
}

static void sokol_rhi_draw_fullscreen_texture(const void* texture) {
    if (!texture) return;
    
    if (texture != s_rhi.last_custom_tex) {
        if (s_rhi.custom_img.id != SG_INVALID_ID) sg_destroy_image(s_rhi.custom_img);
        
        sg_image_desc wrap_desc = {
            .type = SG_IMAGETYPE_2D,
            .width = s_rhi.width, .height = s_rhi.height,
            .pixel_format = SG_PIXELFORMAT_BGRA8,
            .sample_count = 1,
            .wgpu_texture = texture
        };
        s_rhi.custom_img = sg_make_image(&wrap_desc);
        sg_view_desc view_desc = { .texture = { .image = s_rhi.custom_img } };
        s_rhi.custom_view = sg_make_view(&view_desc);
        s_rhi.blit_bind.views[0] = s_rhi.custom_view;
        s_rhi.last_custom_tex = texture;
    }
    
    sg_pass pass = {
        .action = { .colors[0] = { .load_action = SG_LOADACTION_LOAD } },
        .attachments = { .colors[0] = s_rhi.offscreen_view }
    };
    sg_begin_pass(&pass);
    sg_apply_pipeline(s_rhi.blit_pip);
    sg_apply_bindings(&s_rhi.blit_bind);
    sg_draw(0, 4, 1);
    sg_end_pass();
}

/* ------------------------------------------------------------- */
/* API Stubs for Phase 2 */
/* ------------------------------------------------------------- */

static void* sokol_rhi_create_rect(void) {
    RenderNode* node = NULL;
    for (int i = 0; i < s_rhi.node_count; ++i) {
        if (!s_rhi.nodes[i].is_alive) {
            node = &s_rhi.nodes[i];
            break;
        }
    }
    
    if (!node) {
        if (s_rhi.node_count >= MAX_NODES) return NULL;
        node = &s_rhi.nodes[s_rhi.node_count++];
    }
    
    memset(node, 0, sizeof(RenderNode));
    node->is_visible = true;
    node->is_alive = true;
    return (void*)node;
}

static void sokol_rhi_destroy(void* handle) {
    RenderNode* node = (RenderNode*)handle;
    if (node) {
        node->is_alive = false;
    }
}

static void sokol_rhi_translate(void* handle, float x, float y) {
    RenderNode* node = (RenderNode*)handle;
    if (node) { node->x = x; node->y = y; }
}

static void sokol_rhi_set_rect_geometry(void* handle, float w, float h, float rx, float ry) {
    RenderNode* node = (RenderNode*)handle;
    if (node) { node->w = w; node->h = h; node->rx = rx; node->ry = ry; }
}

static void sokol_rhi_set_fill_color(void* handle, Tether_Color color) {
    RenderNode* node = (RenderNode*)handle;
    if (node) { node->color = color; }
}

static void sokol_rhi_set_visible(void* handle, int visible) {
    RenderNode* node = (RenderNode*)handle;
    if (node) { node->is_visible = visible; }
}

/* Stubs for unsupported features (will fallback to ThorVG in Phase 3/4) */
static void sokol_rhi_scale(void* handle, float sx, float sy) {
    // TODO: Phase 2.1 matrix transformations
}

static void sokol_rhi_rotate(void* handle, float rot) {
    // TODO: Phase 2.1 matrix transformations
}

static void sokol_rhi_set_opacity(void* handle, uint8_t opacity) {
    RenderNode* node = (RenderNode*)handle;
    if (node) {
        node->color.a = opacity;
    }
}

static void sokol_rhi_set_z_index(void* handle, int z_index) {
    RenderNode* node = (RenderNode*)handle;
    if (node) {
        node->z_index = z_index;
    }
}

static Tether_System_API sokol_system_api = {
    .init = sokol_rhi_init,
    .resize = sokol_rhi_resize,
    .term = sokol_rhi_term,
    .get_texture = sokol_rhi_get_texture,
    .get_texture_size = sokol_rhi_get_texture_size,
    .draw = sokol_rhi_draw,
    .draw_fullscreen_texture = sokol_rhi_draw_fullscreen_texture,
};

static Tether_Shape_API sokol_shape_api = {
    .create_rect = sokol_rhi_create_rect,
    .set_rect_geometry = sokol_rhi_set_rect_geometry,
    .set_fill_color = sokol_rhi_set_fill_color,
    .set_visible = sokol_rhi_set_visible,
    .set_opacity = sokol_rhi_set_opacity,
    .translate = sokol_rhi_translate,
    .scale = sokol_rhi_scale,
    .rotate = sokol_rhi_rotate,
    .destroy = sokol_rhi_destroy,
    .set_z_index = sokol_rhi_set_z_index,
};

Tether_System_API* sokol_system_get_api(void) {
    return &sokol_system_api;
}

Tether_Shape_API* sokol_shape_get_api(void) {
    return &sokol_shape_api;
}
