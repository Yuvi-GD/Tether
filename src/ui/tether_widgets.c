#include "tether/core/tether_components.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_input.h"
#include "tether/engine/tether_engine.h"
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

void tether_widget_remove_from_parent(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return;
    tether_ecs_detach_entity(entity);
    tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_HIERARCHY);
    tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
}

Tether_GUID tether_widget_get_parent(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return TETHER_INVALID_GUID;
    Tether_Hierarchy *h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    return h ? h->parent : TETHER_INVALID_GUID;
}

int tether_widget_get_child_count(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return 0;
    Tether_Hierarchy *h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    return h ? (int)h->child_count : 0;
}

Tether_GUID tether_widget_get_child_at(Tether_GUID entity, int index) {
    if (entity == TETHER_INVALID_GUID) return TETHER_INVALID_GUID;
    Tether_Hierarchy *h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    if (!h || h->child_count == 0) return TETHER_INVALID_GUID;

    if (index == -1) {
        return h->last_child;
    }
    if (index < 0 || (uint32_t)index >= h->child_count) {
        return TETHER_INVALID_GUID;
    }
    if (index == 0) {
        return h->first_child;
    }

    Tether_GUID curr = h->first_child;
    for (int i = 0; i < index && curr != TETHER_INVALID_GUID; i++) {
        Tether_Hierarchy *ch = (Tether_Hierarchy*)tether_ecs_get_component(curr, TETHER_COMPONENT_HIERARCHY);
        curr = ch ? ch->next_sibling : TETHER_INVALID_GUID;
    }
    return curr;
}


Tether_GUID tether_widget_create_container(Tether_GUID parent) {
    Tether_GUID entity = tether_ecs_create_entity();
    
    tether_ecs_add_component(entity, TETHER_COMPONENT_SLOT_TRANSFORM);
    tether_ecs_add_component(entity, TETHER_COMPONENT_LAYOUT);
    tether_ecs_add_component(entity, TETHER_COMPONENT_VISIBILITY);
    tether_ecs_add_component(entity, TETHER_COMPONENT_ANCHOR_SLOT);
    tether_ecs_add_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    tether_ecs_add_component(entity, TETHER_COMPONENT_RENDER_TRANSFORM);
    tether_ecs_add_component(entity, TETHER_COMPONENT_OVERFLOW);

    tether_widget_add_to_parent(entity, parent);
    tether_widget_set_visibility(entity, TETHER_VISIBLE);

    return entity;
}

void tether_widget_show(Tether_GUID entity) {
    tether_widget_set_visibility(entity, TETHER_VISIBLE);
    tether_engine_realize(entity);
}

void tether_widget_hide(Tether_GUID entity) {
    tether_engine_unrealize(entity);
    tether_widget_set_visibility(entity, TETHER_COLLAPSED);
}

static void clear_bindings_recursive(Tether_GUID entity) {
    if (!tether_ecs_is_valid(entity)) return;
    tether_input_clear_route(entity);
    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    if (h) {
        Tether_GUID child = h->first_child;
        while (child != TETHER_INVALID_GUID) {
            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            Tether_GUID next = ch ? ch->next_sibling : TETHER_INVALID_GUID;
            clear_bindings_recursive(child);
            child = next;
        }
    }
    tether_clear_bindings(entity);
}

void tether_widget_destroy(Tether_GUID entity) {
    if (!tether_ecs_is_valid(entity)) return;
    
    // Safely unrealize GPU resources recursively before the ECS data is gone
    tether_engine_unrealize(entity);
    
    // Clear event bindings recursively across this widget and all its children
    clear_bindings_recursive(entity);

    // Destroy the entity from ECS
    tether_ecs_destroy_entity(entity);
}

    /* ========================================================================== */
    /*                 Visibility Settings for UI                                 */
    /* ========================================================================== */

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

    /* ========================================================================== */
    /*                 Interaction & State Settings                               */
    /* ========================================================================== */

void tether_widget_add_interaction(Tether_GUID entity, Tether_HitBehavior behavior) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Interactable *interact = (Tether_Interactable*)tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (!interact) {
        interact = (Tether_Interactable*)tether_ecs_add_component(entity, TETHER_COMPONENT_INTERACTABLE);
    }
    if (interact) {
        interact->hit_behavior = behavior;
    }
}

/* Compound widgets use existing child entities; dim and disable them together. */
static void apply_enabled(Tether_GUID entity, bool enabled) {
    Tether_Interactable *interaction = tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (interaction) {
        interaction->is_hovered = false;
        interaction->is_pressed = false;
        interaction->has_focus = false;
    }
    Tether_Shape *shape = tether_ecs_get_component(entity, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->active_bg = (Tether_Color){0, 0, 0, 0};
        shape->active_border = (Tether_Color){0, 0, 0, 0};
    }
    Tether_RenderTransform *transform = tether_ecs_get_component(entity, TETHER_COMPONENT_RENDER_TRANSFORM);
    if (transform) {
        transform->opacity = enabled ? transform->opacity * 2.0f : transform->opacity * 0.5f;
        if (transform->opacity > 1.0f) transform->opacity = 1.0f;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
    Tether_Hierarchy *hierarchy = tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    Tether_GUID child = hierarchy ? hierarchy->first_child : TETHER_INVALID_GUID;
    while (child != TETHER_INVALID_GUID) {
        apply_enabled(child, enabled);
        hierarchy = tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
        child = hierarchy ? hierarchy->next_sibling : TETHER_INVALID_GUID;
    }
}

void tether_widget_set_enabled(Tether_GUID entity, bool enabled) {
    if (!tether_ecs_is_valid(entity)) return;
    Tether_Interactable *interaction = tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (!interaction) {
        if (enabled) return;
        interaction = tether_ecs_add_component(entity, TETHER_COMPONENT_INTERACTABLE);
        if (!interaction) return;
        interaction->hit_behavior = TETHER_HIT_CHILD_ONLY;
    }
    if (interaction->is_enabled == enabled) return;
    interaction->is_enabled = enabled;
    apply_enabled(entity, enabled);
}

bool tether_widget_is_enabled(Tether_GUID entity) {
    if (!tether_ecs_is_valid(entity)) return false;
    Tether_Interactable *interaction = tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    return interaction ? interaction->is_enabled : true;
}
