#include "tether/backends/tether_rhi.h"

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"

#include "tether/engine/tether_render.h"
#include "tether/engine/tether_layout.h"


static void* main_scene = NULL;

static inline uint8_t clamp_u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
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

        if (h->scene_handle) {
            /* Detach from parent scene if we have a parent */
            if (h->parent != 0) {
                Tether_Hierarchy* p = (Tether_Hierarchy*)tether_ecs_get_component(h->parent, TETHER_COMPONENT_HIERARCHY);
                if (p && p->scene_handle) {
                    tether_rhi_scene_remove(p->scene_handle, h->scene_handle);
                }
            }
            tether_rhi_paint_free(h->scene_handle);
            h->scene_handle = NULL;
        }
    }

    Tether_Style* style = (Tether_Style*)tether_ecs_get_component(entity, TETHER_COMPONENT_STYLE);
    if (style && style->render_handle) {
        tether_rhi_paint_free(style->render_handle);
        style->render_handle = NULL;
    }

    Tether_Text* text = (Tether_Text*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT);
    if (text && text->text_handle) {
        tether_rhi_paint_free(text->text_handle);
        text->text_handle = NULL;
    }
    
    Tether_Overflow* of = (Tether_Overflow*)tether_ecs_get_component(entity, TETHER_COMPONENT_OVERFLOW);
    if (of && of->clip_handle) {
        tether_rhi_paint_free(of->clip_handle);
        of->clip_handle = NULL;
    }
}

static void apply_visual_rules(Tether_GUID id) {
    Tether_SlotTransform* st = (Tether_SlotTransform*)tether_ecs_get_component(id, TETHER_COMPONENT_SLOT_TRANSFORM);
    Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_get_component(id, TETHER_COMPONENT_RENDER_TRANSFORM);
    Tether_Visibility* vis = (Tether_Visibility*)tether_ecs_get_component(id, TETHER_COMPONENT_VISIBILITY);
    
    Tether_Layout* layout = (Tether_Layout*)tether_ecs_get_component(id, TETHER_COMPONENT_LAYOUT);
    Tether_Style* style = (Tether_Style*)tether_ecs_get_component(id, TETHER_COMPONENT_STYLE);
    Tether_Text* text = (Tether_Text*)tether_ecs_get_component(id, TETHER_COMPONENT_TEXT);
    Tether_Overflow* overflow = (Tether_Overflow*)tether_ecs_get_component(id, TETHER_COMPONENT_OVERFLOW);

    int is_hidden = vis && (vis->computed_state == TETHER_HIDDEN || vis->computed_state == TETHER_COLLAPSED);

    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(id, TETHER_COMPONENT_HIERARCHY);
    if (h && h->scene_handle) {
        tether_rhi_set_visible(h->scene_handle, !is_hidden);
    }

    /* --- STYLE COMPONENT RULE --- */
    if (style) {
        tether_rhi_set_visible(style->render_handle, !is_hidden);
        
        if (!is_hidden) {
            if (st) {
                float rx = style->border_radius.top; 
                float ry = style->border_radius.top;
                tether_rhi_set_rect_geometry(style->render_handle, st->width, st->height, rx, ry);
                
                float final_x = st->x;
                float final_y = st->y;
                float scale_x = 1.0f;
                float scale_y = 1.0f;
                float rot = 0.0f;
                
                if (rt) {
                    final_x += rt->translation_x;
                    final_y += rt->translation_y;
                    scale_x = rt->scale_x;
                    scale_y = rt->scale_y;
                    rot = rt->rotation_deg;
                }
                
                tether_rhi_translate(style->render_handle, final_x, final_y);
                tether_rhi_scale(style->render_handle, scale_x, scale_y);
                tether_rhi_rotate(style->render_handle, rot);
            }
            
            Tether_Color active_color = resolve_color(id, style->bg_color);
            tether_rhi_set_fill_color(style->render_handle, active_color);
            
            if (style->border_width > 0.0f) {
                tether_rhi_set_stroke_color(style->render_handle, style->border_color);
                tether_rhi_set_stroke_width(style->render_handle, style->border_width);
            }
            
            uint8_t final_opacity = active_color.a;
            if (rt) final_opacity = (uint8_t)(final_opacity * rt->opacity);
            tether_rhi_set_opacity(style->render_handle, final_opacity);
        }
    }

    /* --- TEXT COMPONENT RULE --- */
    if (text) {
        tether_rhi_set_visible(text->text_handle, !is_hidden);
        
        if (!is_hidden) {
            const char* str = tether_ecs_get_text_string(id);
            if (str && str[0] != '\0') {
                tether_rhi_set_text_string(text->text_handle, str);
                tether_rhi_set_text_font(text->text_handle, text->font_id, text->font_style, text->font_size);
                tether_rhi_set_text_color(text->text_handle, text->color);
                
                if (st && layout && layout->wrap) {
                    tether_rhi_set_text_wrap(text->text_handle, st->width);
                } else if (st && text->wrap_width > 0.0f) {
                    tether_rhi_set_text_wrap(text->text_handle, text->wrap_width);
                } else {
                    tether_rhi_set_text_wrap(text->text_handle, 0.0f);
                }
            }
            
            if (st) {
                float tx = st->x;
                float ty = st->y;
                
                tether_rhi_translate(text->text_handle, 0.0f, 0.0f);
                float tw = 0.0f, th = 0.0f;
                float t_x = 0.0f, t_y = 0.0f;
                tether_rhi_get_text_bounds(text->text_handle, &t_x, &t_y, &tw, &th);
                
                if (text->align_x == TETHER_ALIGN_CENTER) tx += (st->width - tw) * 0.5f;
                else if (text->align_x == TETHER_ALIGN_END) tx += (st->width - tw);
                
                if (text->align_y == TETHER_ALIGN_CENTER) ty += (st->height - th) * 0.5f;
                else if (text->align_y == TETHER_ALIGN_END) ty += (st->height - th);

                tx -= t_x;
                ty -= t_y;

                float scale_x = 1.0f;
                float scale_y = 1.0f;
                float rot = 0.0f;
                
                if (rt) {
                    tx += rt->translation_x;
                    ty += rt->translation_y;
                    scale_x = rt->scale_x;
                    scale_y = rt->scale_y;
                    rot = rt->rotation_deg;
                }
                
                tether_rhi_translate(text->text_handle, tx, ty);
                tether_rhi_scale(text->text_handle, scale_x, scale_y);
                tether_rhi_rotate(text->text_handle, rot);
            }
            
            uint8_t final_opacity = text->color.a;
            if (rt) final_opacity = (uint8_t)(final_opacity * rt->opacity);
            tether_rhi_set_opacity(text->text_handle, final_opacity);
        }
    }

    /* --- OVERFLOW COMPONENT RULE --- */
    if (overflow && h && h->scene_handle) {
        if (overflow->x == TETHER_OVERFLOW_CLIP || overflow->y == TETHER_OVERFLOW_CLIP) {
            if (!overflow->clip_handle) overflow->clip_handle = tether_rhi_create_rect();
            
            if (st) {
                float rx = style ? style->border_radius.top : 0.0f;
                float ry = style ? style->border_radius.top : 0.0f;
                tether_rhi_set_rect_geometry(overflow->clip_handle, st->width, st->height, rx, ry);
                
                float final_x = st->x;
                float final_y = st->y;
                float scale_x = 1.0f;
                float scale_y = 1.0f;
                float rot = 0.0f;
                
                if (rt) {
                    final_x += rt->translation_x;
                    final_y += rt->translation_y;
                    scale_x = rt->scale_x;
                    scale_y = rt->scale_y;
                    rot = rt->rotation_deg;
                }
                
                tether_rhi_translate(overflow->clip_handle, final_x, final_y);
                tether_rhi_scale(overflow->clip_handle, scale_x, scale_y);
                tether_rhi_rotate(overflow->clip_handle, rot);
            }
            
            tether_rhi_set_clip_rect(h->scene_handle, overflow->clip_handle);
        } else if (overflow->clip_handle) {
            tether_rhi_paint_free(overflow->clip_handle);
            overflow->clip_handle = NULL;
            tether_rhi_set_clip_rect(h->scene_handle, NULL);
        }
    }
}

static void sync_hierarchy_node(Tether_GUID id) {
    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(id, TETHER_COMPONENT_HIERARCHY);
    if (!h) return;
    
    if (!h->scene_handle) {
        h->scene_handle = tether_rhi_create_scene();
    }
    
    /* Now that we have proper ref-counting on paints, we can safely clear the scene.
       Any active children will still be held by ECS, and any orphaned children will be cleanly destroyed. */
    tether_rhi_scene_clear(h->scene_handle);
    
    /* Re-add them in Z-order */
    Tether_GUID child = h->first_child;
    while (child != 0) {
        Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
        if (ch) {
            Tether_Style* ch_style = (Tether_Style*)tether_ecs_get_component(child, TETHER_COMPONENT_STYLE);
            Tether_Text* ch_text = (Tether_Text*)tether_ecs_get_component(child, TETHER_COMPONENT_TEXT);
            
            if (ch_style) tether_rhi_scene_push(h->scene_handle, ch_style->render_handle);
            if (ch_text) tether_rhi_scene_push(h->scene_handle, ch_text->text_handle);
            if (ch->scene_handle) tether_rhi_scene_push(h->scene_handle, ch->scene_handle);
            
            child = ch->next_sibling;
        } else {
            child = 0;
        }
    }
}

void tether_render_init(void) {
    if (!main_scene) {
        main_scene = tether_rhi_create_scene();
        tether_rhi_add_to_canvas(main_scene);
    }
    
    Tether_GUID main_root = tether_ecs_get_main_root();
    Tether_GUID overlay_root = tether_ecs_get_overlay_root();
    
    Tether_Hierarchy* h_main = (Tether_Hierarchy*)tether_ecs_get_component(main_root, TETHER_COMPONENT_HIERARCHY);
    if (h_main) {
        if (!h_main->scene_handle) h_main->scene_handle = tether_rhi_create_scene();
        tether_rhi_scene_push(main_scene, h_main->scene_handle);
    }
    
    Tether_Hierarchy* h_overlay = (Tether_Hierarchy*)tether_ecs_get_component(overlay_root, TETHER_COMPONENT_HIERARCHY);
    if (h_overlay) {
        if (!h_overlay->scene_handle) h_overlay->scene_handle = tether_rhi_create_scene();
        tether_rhi_scene_push(main_scene, h_overlay->scene_handle);
    }
}

bool tether_render_frame(float screen_w, float screen_h, bool force_redraw) {
    Tether_DenseArray* dirty_layout = tether_ecs_get_dense_array(TETHER_COMPONENT_DIRTY_LAYOUT);
    Tether_DenseArray* dirty_hierarchy = tether_ecs_get_dense_array(TETHER_COMPONENT_DIRTY_HIERARCHY);
    Tether_DenseArray* dirty_visual = tether_ecs_get_dense_array(TETHER_COMPONENT_DIRTY_VISUAL);

    /* 1. Process Layout Dirty Entities */
    if (force_redraw || (dirty_layout && dirty_layout->count > 0)) {
        tether_layout_process_all(screen_w, screen_h);
        force_redraw = true;
    }
    
    if (dirty_hierarchy && dirty_hierarchy->count > 0) force_redraw = true;
    if (dirty_visual && dirty_visual->count > 0) force_redraw = true;

    
    /* 2. Sync SceneGraph Hierarchy (Local syncing only) */
    if (dirty_hierarchy) {
        for (uint32_t i = 0; i < dirty_hierarchy->count; i++) {
            Tether_GUID id = dirty_hierarchy->entity_map[i];
            sync_hierarchy_node(id);
        }
    }
    
    /* 3. Process Visual Dirty Entities */
    if (dirty_visual) {
        for (uint32_t i = 0; i < dirty_visual->count; i++) {
            apply_visual_rules(dirty_visual->entity_map[i]);
        }
    }
    
    /* 4. Flush Dirty Arrays (Except Volatile) */
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

    if (force_redraw) {
        tether_rhi_draw();
        return true;
    }
    return false;
}

void tether_render_realize(Tether_GUID id) {
    if (id == TETHER_INVALID_GUID) return;

    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(id, TETHER_COMPONENT_HIERARCHY);
    if (h && !h->scene_handle) {
        h->scene_handle = tether_rhi_create_scene();
    }
    
    Tether_Style* style = (Tether_Style*)tether_ecs_get_component(id, TETHER_COMPONENT_STYLE);
    if (style && !style->render_handle) {
        style->render_handle = tether_rhi_create_rect();
    }
    
    Tether_Text* text = (Tether_Text*)tether_ecs_get_component(id, TETHER_COMPONENT_TEXT);
    if (text && !text->text_handle) {
        text->text_handle = tether_rhi_create_text();
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
