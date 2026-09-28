#include "tether/backends/tether_rhi.h"

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"

#include "tether/engine/tether_render.h"
#include "tether/engine/tether_layout.h"




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
    }

    Tether_Style* style = (Tether_Style*)tether_ecs_get_component(entity, TETHER_COMPONENT_STYLE);
    if (style && style->render_handle) {
        g_rhi.shape->destroy(style->render_handle);
        style->render_handle = NULL;
    }

    Tether_Text* text = (Tether_Text*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT);
    if (text && text->text_handle) {
        g_rhi.text->destroy(text->text_handle);
        text->text_handle = NULL;
    }
    
    Tether_Overflow* of = (Tether_Overflow*)tether_ecs_get_component(entity, TETHER_COMPONENT_OVERFLOW);
    if (of && of->clip_handle) {
        g_rhi.shape->destroy(of->clip_handle);
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

    /* --- STYLE COMPONENT RULE --- */
    if (style) {
        g_rhi.shape->set_visible(style->render_handle, !is_hidden);
        
        if (!is_hidden) {
            if (st) {
                float rx = style->border_radius.top; 
                float ry = style->border_radius.top;
                g_rhi.shape->set_rect_geometry(style->render_handle, st->width, st->height, rx, ry);
                
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
                
                g_rhi.shape->translate(style->render_handle, final_x, final_y);
                g_rhi.shape->scale(style->render_handle, scale_x, scale_y);
                g_rhi.shape->rotate(style->render_handle, rot);
            }
            
            Tether_Color active_color = resolve_color(id, style->bg_color);
            g_rhi.shape->set_fill_color(style->render_handle, active_color);
            
            // TODO: border strock width and color

            uint8_t final_opacity = active_color.a;
            if (rt) final_opacity = (uint8_t)(final_opacity * rt->opacity);
            g_rhi.shape->set_opacity(style->render_handle, final_opacity);
        }
    }

    /* --- TEXT COMPONENT RULE --- */
    if (text) {
        g_rhi.text->set_visible(text->text_handle, !is_hidden);
        
        if (!is_hidden) {
            const char* str = tether_ecs_get_text_string(id);
            if (str && str[0] != '\0') {
                g_rhi.text->set_text_string(text->text_handle, str);
                g_rhi.text->set_text_font(text->text_handle, text->font_id, text->font_style, text->font_size);
                g_rhi.text->set_text_color(text->text_handle, text->color);
                
                if (st && layout && layout->wrap) {
                    g_rhi.text->set_text_wrap(text->text_handle, st->width);
                } else if (st && text->wrap_width > 0.0f) {
                    g_rhi.text->set_text_wrap(text->text_handle, text->wrap_width);
                } else {
                    g_rhi.text->set_text_wrap(text->text_handle, 0.0f);
                }
            }
            
            if (st) {
                float tx = st->x;
                float ty = st->y;
                
                g_rhi.text->translate(text->text_handle, 0.0f, 0.0f);
                float tw = 0.0f, th = 0.0f;
                float t_x = 0.0f, t_y = 0.0f;
                g_rhi.text->get_text_bounds(text->text_handle, &t_x, &t_y, &tw, &th);
                
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
                
                g_rhi.text->translate(text->text_handle, tx, ty);
                g_rhi.text->scale(text->text_handle, scale_x, scale_y);
                g_rhi.text->rotate(text->text_handle, rot);
            }
            
            uint8_t final_opacity = text->color.a;
            if (rt) final_opacity = (uint8_t)(final_opacity * rt->opacity);
            g_rhi.text->set_opacity(text->text_handle, final_opacity);
        }
    }

    /* --- OVERFLOW COMPONENT RULE --- */
    if (overflow && h) {
        if (overflow->x == TETHER_OVERFLOW_CLIP || overflow->y == TETHER_OVERFLOW_CLIP) {
            if (!overflow->clip_handle) overflow->clip_handle = g_rhi.shape->create_rect();
            
            if (st) {
                float rx = style ? style->border_radius.top : 0.0f;
                float ry = style ? style->border_radius.top : 0.0f;
                g_rhi.shape->set_rect_geometry(overflow->clip_handle, st->width, st->height, rx, ry);
                
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
                
                g_rhi.shape->translate(overflow->clip_handle, final_x, final_y);
                g_rhi.shape->scale(overflow->clip_handle, scale_x, scale_y);
                g_rhi.shape->rotate(overflow->clip_handle, rot);
            }
            
            /* TODO: native clipping */
        } else if (overflow->clip_handle) {
            g_rhi.shape->destroy(overflow->clip_handle);
            overflow->clip_handle = NULL;
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

extern Tether_GUID tether_ecs_get_main_root(void);

static void sync_z_index_recursive(Tether_GUID id, int* current_z) {
    if (id == TETHER_INVALID_GUID) return;

    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(id, TETHER_COMPONENT_HIERARCHY);
    Tether_Style* style = (Tether_Style*)tether_ecs_get_component(id, TETHER_COMPONENT_STYLE);
    Tether_Text* text = (Tether_Text*)tether_ecs_get_component(id, TETHER_COMPONENT_TEXT);

    int z = (*current_z)++;
    if (style && style->render_handle && g_rhi.shape->set_z_index) {
        g_rhi.shape->set_z_index(style->render_handle, z);
    }
    if (text && text->text_handle && g_rhi.text->set_z_index) {
        g_rhi.text->set_z_index(text->text_handle, z);
    }

    if (h) {
        Tether_GUID child = h->first_child;
        while (child != TETHER_INVALID_GUID) {
            sync_z_index_recursive(child, current_z);
            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            if (ch) child = ch->next_sibling;
            else break;
        }
    }
}

bool tether_render_frame(float screen_w, float screen_h, bool force_redraw) {
    Tether_DenseArray* dirty_layout = tether_ecs_get_dense_array(TETHER_COMPONENT_DIRTY_LAYOUT);
    Tether_DenseArray* dirty_hierarchy = tether_ecs_get_dense_array(TETHER_COMPONENT_DIRTY_HIERARCHY);
    Tether_DenseArray* dirty_visual = tether_ecs_get_dense_array(TETHER_COMPONENT_DIRTY_VISUAL);

    if (dirty_hierarchy && dirty_hierarchy->count > 0) {
        int z = 0;
        sync_z_index_recursive(tether_ecs_get_main_root(), &z);
    }

    /* 1. Process Layout Dirty Entities */
    if (force_redraw || (dirty_layout && dirty_layout->count > 0)) {
        tether_layout_process_all(screen_w, screen_h);
        force_redraw = true;
    }
    
    if (dirty_hierarchy && dirty_hierarchy->count > 0) force_redraw = true;
    if (dirty_visual && dirty_visual->count > 0) force_redraw = true;

    
    /* 2. Process Visual Dirty Entities */
    if (dirty_visual) {
        for (uint32_t i = 0; i < dirty_visual->count; i++) {
            apply_visual_rules(dirty_visual->entity_map[i]);
        }
    }
    
    /* 3. Flush Dirty Arrays (Except Volatile) */
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
        g_rhi.system->draw();
        
        /* Temporary text composite via ThorVG */
        extern Tether_System_API* thorvg_system_get_api(void);
        Tether_System_API* tvg = thorvg_system_get_api();
        tvg->draw();
        if (g_rhi.system->draw_fullscreen_texture) {
            g_rhi.system->draw_fullscreen_texture(tvg->get_texture());
        }
        
        return true;
    }
    return false;
}

void tether_render_realize(Tether_GUID id) {
    if (id == TETHER_INVALID_GUID) return;

    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(id, TETHER_COMPONENT_HIERARCHY);
    Tether_Style* style = (Tether_Style*)tether_ecs_get_component(id, TETHER_COMPONENT_STYLE);
    if (style && !style->render_handle) {
        style->render_handle = g_rhi.shape->create_rect();
    }
    
    Tether_Text* text = (Tether_Text*)tether_ecs_get_component(id, TETHER_COMPONENT_TEXT);
    if (text && !text->text_handle) {
        text->text_handle = g_rhi.text->create_text();
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
