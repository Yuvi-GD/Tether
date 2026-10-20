#include "tether/backends/tether_backend.h"

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"

#include "tether/engine/tether_render.h"
#include "tether/engine/tether_layout.h"

#include <stdbool.h>

static inline uint8_t clamp_u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static inline uint32_t pack_color(Tether_Color c, float opacity) {
    uint8_t a = (uint8_t)(c.a * opacity);
    return ((uint32_t)c.r) | (((uint32_t)c.g) << 8) | (((uint32_t)c.b) << 16) | (((uint32_t)a) << 24);
}

static Tether_Color resolve_color(Tether_GUID entity, Tether_Color base_color) {
    Tether_Interactable* i = (Tether_Interactable*)tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (!i) return base_color;

    if (i->is_pressed) {
        if (i->press_color_mode == TETHER_COLOR_MODE_MANUAL) return i->press_color;
        if (i->press_color_mode == TETHER_COLOR_MODE_AUTO) {
            return (Tether_Color){
                clamp_u8((int)base_color.r - 30),
                clamp_u8((int)base_color.g - 30),
                clamp_u8((int)base_color.b - 30),
                base_color.a
            };
        }
    }

    if (i->is_hovered) {
        if (i->hover_color_mode == TETHER_COLOR_MODE_MANUAL) return i->hover_color;
        if (i->hover_color_mode == TETHER_COLOR_MODE_AUTO) {
            return (Tether_Color){
                clamp_u8((int)base_color.r + 30),
                clamp_u8((int)base_color.g + 30),
                clamp_u8((int)base_color.b + 30),
                base_color.a
            };
        }
    }

    return base_color;
}

static void apply_visual_rules(Tether_GUID id) {
    Tether_SlotTransform* st = (Tether_SlotTransform*)tether_ecs_get_component(id, TETHER_COMPONENT_SLOT_TRANSFORM);
    Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_get_component(id, TETHER_COMPONENT_RENDER_TRANSFORM);
    Tether_Visibility* vis = (Tether_Visibility*)tether_ecs_get_component(id, TETHER_COMPONENT_VISIBILITY);
    
    Tether_Layout* layout = (Tether_Layout*)tether_ecs_get_component(id, TETHER_COMPONENT_LAYOUT);
    Tether_Shape* shape = (Tether_Shape*)tether_ecs_get_component(id, TETHER_COMPONENT_SHAPE);
    Tether_Text* text = (Tether_Text*)tether_ecs_get_component(id, TETHER_COMPONENT_TEXT);

    int is_hidden = vis && (vis->computed_state == TETHER_HIDDEN || vis->computed_state == TETHER_COLLAPSED);
    float opacity = rt ? rt->opacity : 1.0f;

    /* --- SHAPE COMPONENT RULE --- */
    if (shape && g_rhi.shape->is_alive(shape->handle)) {
        if (is_hidden) {
            g_rhi.shape->set_opacity(shape->handle, 0.0f);
        } else {
            float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
            if (st) {
                x = st->x;
                y = st->y;
                w = st->width;
                h = st->height;
            }
            if (rt) {
                x += rt->translation_x;
                y += rt->translation_y;
                w *= rt->scale_x;
                h *= rt->scale_y;
            }
            
            float radius = shape->border_radius.top;
            float border = shape->border_width;

            Tether_Color bg = resolve_color(id, shape->bg_color);
            Tether_Color bc = resolve_color(id, shape->border_color);

            uint32_t fill_u32 = pack_color(bg, opacity);
            uint32_t border_u32 = pack_color(bc, opacity);

            g_rhi.shape->set_bounds(shape->handle, x, y, w, h);
            g_rhi.shape->set_radius(shape->handle, radius);
            g_rhi.shape->set_border(shape->handle, border);
            g_rhi.shape->set_color(shape->handle, fill_u32, border_u32);

            if (rt && rt->rotation_deg != 0.0f) {
                float px = rt->render_pivot.x;
                float py = rt->render_pivot.y;
                float origin_x = x + w * px;
                float origin_y = y + h * py;
                float angle_rad = rt->rotation_deg * (3.14159265358979323846f / 180.0f);
                g_rhi.shape->set_rotation(shape->handle, origin_x, origin_y, angle_rad);
            }
        }
    }

    /* --- TEXT COMPONENT RULE --- */
    if (text && g_rhi.text->is_alive(text->handle)) {
        if (is_hidden) {
            g_rhi.text->set_opacity(text->handle, 0.0f);
        } else {
            const char* str = tether_ecs_get_text_string(id);
            if (!str) str = "";

            float wrap_width = 0.0f;
            if (st && layout && layout->wrap) {
                wrap_width = st->width;
            } else if (text->wrap_width > 0.0f) {
                wrap_width = text->wrap_width;
            }

            float tx = st ? st->x : 0.0f;
            float ty = st ? st->y : 0.0f;

            /* Use layout-computed intrinsic size */
            float tw = text->intrinsic_size.x;
            float th = text->intrinsic_size.y;
            
            if (st) {
                if (text->align_x == TETHER_ALIGN_CENTER) tx += (st->width - tw) * 0.5f;
                else if (text->align_x == TETHER_ALIGN_END) tx += (st->width - tw);

                if (text->align_y == TETHER_ALIGN_CENTER) ty += (st->height - th) * 0.5f;
                else if (text->align_y == TETHER_ALIGN_END) ty += (st->height - th);
            }

            if (rt) {
                tx += rt->translation_x;
                ty += rt->translation_y;
            }

            uint32_t text_color_u32 = pack_color(text->color, opacity);

            g_rhi.text->set_text(text->handle, text->font_id, text->font_size, str, wrap_width, tx, ty);
            g_rhi.text->set_color(text->handle, text_color_u32);

            if (rt) {
                float px = rt->render_pivot.x;
                float py = rt->render_pivot.y;
                float origin_x = (st ? (st->x + rt->translation_x) : tx) + (st ? st->width : tw) * px;
                float origin_y = (st ? (st->y + rt->translation_y) : ty) + (st ? st->height : th) * py;

                if (rt->scale_x != 1.0f || rt->scale_y != 1.0f) {
                    g_rhi.text->set_scale(text->handle, origin_x, origin_y, rt->scale_x, rt->scale_y);
                }
                if (rt->rotation_deg != 0.0f) {
                    float angle_rad = rt->rotation_deg * (3.14159265358979323846f / 180.0f);
                    g_rhi.text->set_rotation(text->handle, origin_x, origin_y, angle_rad);
                }
            }
        }
    }
}

extern Tether_GUID tether_ecs_get_main_root(void);

static void sync_hierarchy_recursive(Tether_GUID id, int* current_z, uint16_t parent_clip_id) {
    if (id == TETHER_INVALID_GUID) return;

    Tether_Hierarchy* h       = (Tether_Hierarchy*)tether_ecs_get_component(id, TETHER_COMPONENT_HIERARCHY);
    Tether_Shape* shape       = (Tether_Shape*)tether_ecs_get_component(id, TETHER_COMPONENT_SHAPE);
    Tether_Text* text         = (Tether_Text*)tether_ecs_get_component(id, TETHER_COMPONENT_TEXT);
    Tether_SlotTransform* st  = (Tether_SlotTransform*)tether_ecs_get_component(id, TETHER_COMPONENT_SLOT_TRANSFORM);
    Tether_Overflow* overflow = (Tether_Overflow*)tether_ecs_get_component(id, TETHER_COMPONENT_OVERFLOW);

    int z = (*current_z)++;
    if (shape && g_rhi.shape->is_alive(shape->handle)) {
        g_rhi.system->set_draw_order(shape->handle, (uint32_t)z);
        g_rhi.system->set_clip(shape->handle, parent_clip_id);
    }
    if (text && g_rhi.text->is_alive(text->handle)) {
        int text_z = (shape && g_rhi.shape->is_alive(shape->handle)) ? (*current_z)++ : z;
        g_rhi.system->set_draw_order(text->handle, (uint32_t)text_z);
        g_rhi.system->set_clip(text->handle, parent_clip_id);
    }

    uint16_t child_clip_id = parent_clip_id;

    if (overflow && (overflow->x == TETHER_OVERFLOW_CLIP || overflow->y == TETHER_OVERFLOW_CLIP) && st) {
        /* Compute inner content box: cut padding so children are clipped inside the border */
        float inner_x = st->x;
        float inner_y = st->y;
        float inner_w = st->width;
        float inner_h = st->height;

        Tether_Layout* layout = (Tether_Layout*)tether_ecs_get_component(id, TETHER_COMPONENT_LAYOUT);
        if (layout) {
            inner_x += layout->padding.left;
            inner_y += layout->padding.top;
            inner_w -= (layout->padding.left + layout->padding.right);
            inner_h -= (layout->padding.top  + layout->padding.bottom);
            if (inner_w < 0.0f) inner_w = 0.0f;
            if (inner_h < 0.0f) inner_h = 0.0f;
        }

        /* Only register / intersect if there are children to clip */
        if (h && h->first_child != TETHER_INVALID_GUID) {
            /* Read the previously-allocated child clip_id from the first child's handle sort key (O(1)) */
            uint16_t existing_clip_id = 0;
            Tether_Shape* fcs = (Tether_Shape*)tether_ecs_get_component(h->first_child, TETHER_COMPONENT_SHAPE);
            Tether_Text*  fct = (Tether_Text*) tether_ecs_get_component(h->first_child, TETHER_COMPONENT_TEXT);
            if (fcs && g_rhi.shape->is_alive(fcs->handle))
                existing_clip_id = g_rhi.system->get_clip_id(fcs->handle);
            else if (fct && g_rhi.text->is_alive(fct->handle))
                existing_clip_id = g_rhi.system->get_clip_id(fct->handle);

            /* First frame: registers a new clip slot. Every frame after: updates the rect in-place. O(1) always. */
            child_clip_id = g_rhi.system->clip_intersect_to(
                existing_clip_id, parent_clip_id,
                (int)inner_x, (int)inner_y, (int)inner_w, (int)inner_h);
        }
    }

    if (h) {
        Tether_GUID child = h->first_child;
        while (child != TETHER_INVALID_GUID) {
            sync_hierarchy_recursive(child, current_z, child_clip_id);
            Tether_Hierarchy* ch_comp = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            if (ch_comp) child = ch_comp->next_sibling;
            else break;
        }
    }
}

void tether_render_init(void) {
    /* No scene graph to init anymore */
}

void tether_render_term(void) {
    /* Release widget-owned backend handles while their ECS components and RHI are valid. */
    tether_render_unrealize(tether_ecs_get_main_root());
    tether_render_unrealize(tether_ecs_get_overlay_root());
}

bool tether_render_frame(float screen_w, float screen_h, bool force_redraw) {
    Tether_DenseArray* dirty_layout = tether_ecs_get_dense_array(TETHER_COMPONENT_DIRTY_LAYOUT);
    Tether_DenseArray* dirty_hierarchy = tether_ecs_get_dense_array(TETHER_COMPONENT_DIRTY_HIERARCHY);
    Tether_DenseArray* dirty_visual = tether_ecs_get_dense_array(TETHER_COMPONENT_DIRTY_VISUAL);

    /* ========================================================================== */
    /*                 Update Dirty UI (if any)                                   */
    /* ========================================================================== */
    
    static float last_screen_w = 0, last_screen_h = 0;
    bool resized = (screen_w != last_screen_w || screen_h != last_screen_h);
    last_screen_w = screen_w; last_screen_h = screen_h;
    
    bool layout_changed = resized || (dirty_layout && dirty_layout->count > 0);

    if (dirty_hierarchy && dirty_hierarchy->count > 0) force_redraw = true;
    if (dirty_visual && dirty_visual->count > 0) force_redraw = true;
    
    /* 1. Process Layout Dirty Entities first so SlotTransforms are updated */
    if (layout_changed) {
        tether_layout_process_all(screen_w, screen_h);
        force_redraw = true;
    }

    /* 2. Sync Hierarchy & Clipping (reads up-to-date SlotTransforms for overflow clip_intersect) */
    if (layout_changed || (dirty_hierarchy && dirty_hierarchy->count > 0)) {
        int z = 0;
        sync_hierarchy_recursive(tether_ecs_get_main_root(), &z, 0);
        sync_hierarchy_recursive(tether_ecs_get_overlay_root(), &z, 0);
    }
    
    /* Process Visual Dirty Entities */
    if (dirty_visual) {
        for (uint32_t i = 0; i < dirty_visual->count; i++) {
            apply_visual_rules(dirty_visual->entity_map[i]);
        }
    }
    
    /* ========================================================================== */
    /*             Flush Dirty Arrays (Except Volatile)                           */
    /* ========================================================================== */

    if (dirty_layout) {
        for (int i = (int)dirty_layout->count - 1; i >= 0; i--) {
            Tether_GUID id = dirty_layout->entity_map[i];
            if (!tether_ecs_get_component(id, TETHER_COMPONENT_VOLATILE)) {
                tether_ecs_remove_component(id, TETHER_COMPONENT_DIRTY_LAYOUT);
            }
        }
    }
    
    if (dirty_visual) {
        for (int i = (int)dirty_visual->count - 1; i >= 0; i--) {
            Tether_GUID id = dirty_visual->entity_map[i];
            if (!tether_ecs_get_component(id, TETHER_COMPONENT_VOLATILE)) {
                tether_ecs_remove_component(id, TETHER_COMPONENT_DIRTY_VISUAL);
            }
        }
    }
    
    if (dirty_hierarchy) {
        for (int i = (int)dirty_hierarchy->count - 1; i >= 0; i--) {
            Tether_GUID id = dirty_hierarchy->entity_map[i];
            tether_ecs_remove_component(id, TETHER_COMPONENT_DIRTY_HIERARCHY);
        }
    }

    /* ========================================================================== */
    /*                 Draw (if any)                                              */
    /* ========================================================================== */

    if (force_redraw) {
        g_rhi.system->mark_dirty();
        return true;
    }
    return false;
}

/* ========================================================================== */
/*                    Add/Remove UI Element from Render                       */
/* ========================================================================== */

void tether_render_realize(Tether_GUID id) {
    if (id == TETHER_INVALID_GUID) return;

    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(id, TETHER_COMPONENT_HIERARCHY);
    Tether_Shape* shape = (Tether_Shape*)tether_ecs_get_component(id, TETHER_COMPONENT_SHAPE);
    if (shape && !g_rhi.shape->is_alive(shape->handle)) {
        shape->handle = g_rhi.shape->create();
    }
    
    Tether_Text* text = (Tether_Text*)tether_ecs_get_component(id, TETHER_COMPONENT_TEXT);
    if (text && !g_rhi.text->is_alive(text->handle)) {
        text->handle = g_rhi.text->create();
    }
    
    tether_ecs_add_component(id, TETHER_COMPONENT_DIRTY_VISUAL);
    tether_ecs_add_component(id, TETHER_COMPONENT_DIRTY_LAYOUT);
    
    /* Recursively realize children */
    if (h) {
        Tether_GUID child = h->first_child;
        while (child != TETHER_INVALID_GUID) {
            tether_render_realize(child);
            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            if (ch) {
                child = ch->next_sibling;
            } else {
                break;
            }
        }
    }
    
    /* Post-traversal: flag the parent for hierarchy sync so this entire subtree is mounted */
    if (h && h->parent != TETHER_INVALID_GUID) {
        tether_ecs_add_component(h->parent, TETHER_COMPONENT_DIRTY_HIERARCHY);
    } else {
        /* If it's a root entity, flag itself so sync_main_scene picks it up */
        tether_ecs_add_component(id, TETHER_COMPONENT_DIRTY_HIERARCHY);
    }
}

void tether_render_unrealize(Tether_GUID entity) {
    if (!tether_ecs_is_valid(entity)) return;

    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    if (h) {
        Tether_GUID child = h->first_child;
        while (child != 0) {
            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            Tether_GUID next = ch ? ch->next_sibling : 0;
            tether_render_unrealize(child);
            child = next;
        }
    }

    Tether_Shape* shape = (Tether_Shape*)tether_ecs_get_component(entity, TETHER_COMPONENT_SHAPE);
    if (shape && g_rhi.shape->is_alive(shape->handle)) {
        g_rhi.system->release_clip(shape->handle);
        g_rhi.shape->destroy(shape->handle);
        shape->handle = TETHER_INVALID_HANDLE;
    }

    Tether_Text* text = (Tether_Text*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT);
    if (text && g_rhi.text->is_alive(text->handle)) {
        g_rhi.system->release_clip(text->handle);
        g_rhi.text->destroy(text->handle);
        text->handle = TETHER_INVALID_HANDLE;
    }
}
