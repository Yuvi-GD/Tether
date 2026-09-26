#include "tether/core/tether_components.h"
#include "tether/engine/tether_render.h"
#include "tether/ui/tether_widgets.h"


void tether_widget_add_to_parent(Tether_GUID entity, Tether_GUID parent) {
    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    if (!h) {
        h = (Tether_Hierarchy*)tether_ecs_add_component(entity, TETHER_COMPONENT_HIERARCHY);
    }
    h->parent = parent;
    h->first_child = TETHER_INVALID_GUID;
    h->next_sibling = TETHER_INVALID_GUID;
    h->child_count = 0;
    
    if (parent != TETHER_INVALID_GUID) {
        Tether_Hierarchy* ph = (Tether_Hierarchy*)tether_ecs_get_component(parent, TETHER_COMPONENT_HIERARCHY);
        if (ph) {
            ph->child_count++;
            if (ph->first_child == TETHER_INVALID_GUID) {
                ph->first_child = entity;
                ph->last_child = entity;
            } else {
                Tether_GUID old_last = ph->last_child;
                Tether_Hierarchy* old_last_h = (Tether_Hierarchy*)tether_ecs_get_component(old_last, TETHER_COMPONENT_HIERARCHY);
                if (old_last_h) {
                    old_last_h->next_sibling = entity;
                    h->prev_sibling = old_last;
                }
                ph->last_child = entity;
            }
        }
    }
}

static Tether_GUID tether_widget_create_base(Tether_GUID parent) {
    Tether_GUID entity = tether_ecs_create_entity();
    
    /* Output: SlotTransform */
    Tether_SlotTransform* st = (Tether_SlotTransform*)tether_ecs_add_component(entity, TETHER_COMPONENT_SLOT_TRANSFORM);
    st->x = 0; st->y = 0; st->width = 100; st->height = 100;
    
    /* Defaults */
    Tether_Layout* node = (Tether_Layout*)tether_ecs_add_component(entity, TETHER_COMPONENT_LAYOUT);
    node->flow = TETHER_FLOW_NONE;
    node->content_align_x = TETHER_ALIGN_FILL;
    node->content_align_y = TETHER_ALIGN_FILL;
    node->padding.top = 0; node->padding.bottom = 0; node->padding.left = 0; node->padding.right = 0;
    node->gap.x = 0; node->gap.y = 0;
    node->size_box.x = 0; node->size_box.y = 0;
    node->wrap = 0;
    
    tether_ecs_add_component(entity, TETHER_COMPONENT_VOLATILE);
    
    Tether_Interactable* i = (Tether_Interactable*)tether_ecs_add_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (i) {
        i->is_hovered = 0;
        i->is_pressed = 0;
        i->has_focus = 0;
        i->hit_behavior = TETHER_HIT_BLOCK; /* Widgets catch hits by default */
        i->hover_color_mode = TETHER_COLOR_MODE_NONE;
        i->press_color_mode = TETHER_COLOR_MODE_NONE;
        i->hover_color = (Tether_Color){0, 0, 0, 0};
        i->press_color = (Tether_Color){0, 0, 0, 0};
    }
    
    Tether_Visibility* vis = (Tether_Visibility*)tether_ecs_add_component(entity, TETHER_COMPONENT_VISIBILITY);
    vis->local_state = TETHER_VISIBLE;
    vis->computed_state = TETHER_VISIBLE;

    Tether_AnchorSlot* anchor = (Tether_AnchorSlot*)tether_ecs_add_component(entity, TETHER_COMPONENT_ANCHOR_SLOT);
    anchor->anchor_min.x = 0; anchor->anchor_min.y = 0;
    anchor->anchor_max.x = 1; anchor->anchor_max.y = 1;
    anchor->offset.top = 0; anchor->offset.bottom = 0; anchor->offset.left = 0; anchor->offset.right = 0;
    anchor->pivot.x = 0; anchor->pivot.y = 0;

    Tether_FlexSlot* flex = (Tether_FlexSlot*)tether_ecs_add_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    flex->margin.top = 0; flex->margin.bottom = 0; flex->margin.left = 0; flex->margin.right = 0;
    flex->fill_ratio = 0.0f;
    flex->align_self_x = TETHER_ALIGN_AUTO;
    flex->align_self_y = TETHER_ALIGN_AUTO;

    Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_add_component(entity, TETHER_COMPONENT_RENDER_TRANSFORM);
    rt->translation_x = 0; rt->translation_y = 0; rt->scale_x = 1.0f; rt->scale_y = 1.0f;
    rt->rotation_deg = 0; rt->pivot_x = 0.5f; rt->pivot_y = 0.5f;
    rt->opacity = 1.0f;

    tether_widget_add_to_parent(entity, parent);
    tether_widget_set_visibility(entity, TETHER_VISIBLE);

    return entity;
}

Tether_GUID tether_widget_create_panel(Tether_GUID parent) {
    Tether_GUID entity = tether_widget_create_base(parent);
    
    Tether_Style* s = (Tether_Style*)tether_ecs_add_component(entity, TETHER_COMPONENT_STYLE);
    if (s) {
        s->bg_color = (Tether_Color){0, 0, 0, 0};
        s->border_color.r = 0; s->border_color.g = 0; s->border_color.b = 0; s->border_color.a = 0;
        s->border_width = 0.0f;
        s->border_radius.top = 0.0f; s->border_radius.right = 0.0f;
        s->border_radius.bottom = 0.0f; s->border_radius.left = 0.0f;
    }
    
    Tether_Overflow* of = (Tether_Overflow*)tether_ecs_add_component(entity, TETHER_COMPONENT_OVERFLOW);
    if (of) {
        of->x = TETHER_OVERFLOW_CLIP;
        of->y = TETHER_OVERFLOW_CLIP;
        of->clip_handle = NULL;
    }
    
    return entity;
}

Tether_GUID tether_widget_create_text(Tether_GUID parent) {
    Tether_GUID entity = tether_widget_create_base(parent);

    Tether_Text* t = (Tether_Text*)tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT);
    t->font_size = 24.0f;
    t->font_id = 1;
    t->font_style = TETHER_FONT_NORMAL;
    t->align_x = TETHER_ALIGN_START;
    t->align_y = TETHER_ALIGN_START;
    t->wrap_width = 0.0f;
    t->color = (Tether_Color){0, 0, 0, 255}; /* Default black */
    t->intrinsic_size.x = 0.0f;
    t->intrinsic_size.y = 0.0f;
    
    /* Text widgets need a place to store their string data */
    /* Text defaults to shrink-wrap layout */
    Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    if (f) f->fill_ratio = 0.0f;
    
    /* Text defaults to ignoring hits so it doesn't block buttons */
    Tether_Interactable* i = (Tether_Interactable*)tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (i) i->hit_behavior = TETHER_HIT_IGNORE;
    
    return entity;
}

Tether_GUID tether_widget_create_button(Tether_GUID parent) {
    Tether_GUID entity = tether_widget_create_panel(parent);

    Tether_Layout* node = (Tether_Layout*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    if (node) {
        node->content_align_x = TETHER_ALIGN_CENTER;
        node->content_align_y = TETHER_ALIGN_CENTER;
        node->padding.top = 10; node->padding.bottom = 10;
        node->padding.left = 20; node->padding.right = 20;
    }
    
    Tether_Interactable* i = (Tether_Interactable*)tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (i) {
        i->hover_color_mode = TETHER_COLOR_MODE_AUTO;
        i->press_color_mode = TETHER_COLOR_MODE_AUTO;
    }
    
    Tether_Style* s = (Tether_Style*)tether_ecs_get_component(entity, TETHER_COMPONENT_STYLE);
    if (s) {
        s->bg_color = (Tether_Color){97, 175, 239, 255}; /* Blueish default */
    }
    
    /* Dynamically add a child Text widget to complete the Button natively */
    Tether_GUID text_entity = tether_widget_create_text(entity);
    Tether_Text* ts = (Tether_Text*)tether_ecs_get_component(text_entity, TETHER_COMPONENT_TEXT);
    if (ts) {
        ts->font_size = 18.0f;
        ts->color = (Tether_Color){255, 255, 255, 255}; /* White text */
    }
    tether_ecs_set_text_string(text_entity, "Button");
    tether_ecs_add_component(entity, TETHER_COMPONENT_VOLATILE);
    return entity;
}

void tether_widget_show(Tether_GUID entity) {
    /* To show a widget, we must allocate its RHI handles. */
    /* This function delegates to the render engine. */
    tether_render_realize(entity);
}

void tether_widget_destroy(Tether_GUID entity) {
    if (!tether_ecs_is_valid(entity)) return;
    
    // Safely unrealize GPU resources recursively before the ECS data is gone
    tether_render_unrealize(entity);
    
    // Destroy the entity from ECS
    tether_ecs_destroy_entity(entity);
}

static void recompute_visibility_tree(Tether_GUID entity, Tether_VisibilityState parent_computed_state) {
    if (!tether_ecs_is_valid(entity)) return;

    Tether_Visibility* vis = (Tether_Visibility*)tether_ecs_get_component(entity, TETHER_COMPONENT_VISIBILITY);
    if (!vis) return;

    Tether_VisibilityState old_computed = vis->computed_state;
    
    if (parent_computed_state != TETHER_VISIBLE) {
        vis->computed_state = parent_computed_state;
    } else {
        vis->computed_state = vis->local_state;
    }
    
    if (vis->computed_state != old_computed) {
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
        
        if (vis->computed_state == TETHER_COLLAPSED || old_computed == TETHER_COLLAPSED) {
            tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
        }
    }

    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    if (h) {
        Tether_GUID child = h->first_child;
        while (child != TETHER_INVALID_GUID) {
            recompute_visibility_tree(child, vis->computed_state);
            
            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            if (ch) {
                child = ch->next_sibling;
            } else {
                child = TETHER_INVALID_GUID;
            }
        }
    }
}

void tether_widget_set_visibility(Tether_GUID entity, Tether_VisibilityState state) {
    Tether_Visibility* vis = (Tether_Visibility*)tether_ecs_get_component(entity, TETHER_COMPONENT_VISIBILITY);
    if (!vis) return;
    
    vis->local_state = state;
    
    Tether_VisibilityState parent_state = TETHER_VISIBLE;
    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    if (h && h->parent != TETHER_INVALID_GUID) {
        Tether_Visibility* p_vis = (Tether_Visibility*)tether_ecs_get_component(h->parent, TETHER_COMPONENT_VISIBILITY);
        if (p_vis) {
            parent_state = p_vis->computed_state;
        }
    }
    
    recompute_visibility_tree(entity, parent_state);
}

bool tether_widget_is_visible(Tether_GUID entity) {
    Tether_Visibility* vis = (Tether_Visibility*)tether_ecs_get_component(entity, TETHER_COMPONENT_VISIBILITY);
    if (!vis) return true; // Default to visible if no component
    return vis->computed_state == TETHER_VISIBLE;
}

bool tether_widget_is_collapsed(Tether_GUID entity) {
    Tether_Visibility* vis = (Tether_Visibility*)tether_ecs_get_component(entity, TETHER_COMPONENT_VISIBILITY);
    if (!vis) return false;
    return vis->computed_state == TETHER_COLLAPSED;
}
