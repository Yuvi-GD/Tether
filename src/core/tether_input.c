#include "tether/core/tether_ecs.h"
#include "tether/core/tether_input.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_components.h"


static Tether_GUID g_hovered = TETHER_INVALID_GUID;
static Tether_GUID g_pressed = TETHER_INVALID_GUID;
static Tether_GUID g_route = TETHER_INVALID_GUID;
static Tether_InputRouteCallback g_route_callback;
static Tether_GUID g_right_pressed = TETHER_INVALID_GUID;
static float g_pointer_x = 0.0f;
static float g_pointer_y = 0.0f;

void tether_input_set_route(Tether_GUID entity, Tether_InputRouteCallback callback) {
    g_route = entity;
    g_route_callback = callback;
}

void tether_input_clear_route(Tether_GUID entity) {
    if (g_route == entity) {
        g_route = TETHER_INVALID_GUID;
        g_route_callback = NULL;
    }
}

void tether_input_get_pointer_pos(float* out_x, float* out_y) {
    if (out_x) *out_x = g_pointer_x;
    if (out_y) *out_y = g_pointer_y;
}

static Tether_GUID hit_test_tree(Tether_GUID entity, float x, float y) {
    if (!tether_ecs_is_valid(entity)) return TETHER_INVALID_GUID;

    Tether_Interactable* interact = tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (interact && !interact->is_enabled) return TETHER_INVALID_GUID;
    Tether_Layout* node = (Tether_Layout*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    Tether_Visibility* vis = (Tether_Visibility*)tether_ecs_get_component(entity, TETHER_COMPONENT_VISIBILITY);
    if (vis && (vis->computed_state == TETHER_HIDDEN || vis->computed_state == TETHER_COLLAPSED)) {
        return TETHER_INVALID_GUID;
    }

    Tether_SlotTransform* st = (Tether_SlotTransform*)tether_ecs_get_component(entity, TETHER_COMPONENT_SLOT_TRANSFORM);
    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);

    /* If entity is a bare root (no layout/transform), just recurse into children */
    if (!node || !st) {
        if (h && h->last_child != TETHER_INVALID_GUID) {
            Tether_GUID child = h->last_child;
            while (child != TETHER_INVALID_GUID) {
                Tether_GUID hit = hit_test_tree(child, x, y);
                if (hit != TETHER_INVALID_GUID) return hit;
                Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
                child = ch ? ch->prev_sibling : TETHER_INVALID_GUID;
            }
        }
        return TETHER_INVALID_GUID;
    }

    /* 1. O(1) Spatial Culling: If mouse is outside this node, only check position absolute children */
    bool inside_self = (x >= st->x && x <= st->x + st->width && y >= st->y && y <= st->y + st->height);
    if (!inside_self) {
        if (h && h->last_child != TETHER_INVALID_GUID) {
            Tether_GUID child = h->last_child;
            while (child != TETHER_INVALID_GUID) {
                Tether_Layout* child_layout = (Tether_Layout*)tether_ecs_get_component(child, TETHER_COMPONENT_LAYOUT);
                if (child_layout && (child_layout->position == TETHER_POSITION_ABSOLUTE || child_layout->position == TETHER_POSITION_FIXED)) {
                    Tether_GUID hit = hit_test_tree(child, x, y);
                    if (hit != TETHER_INVALID_GUID) return hit;
                }
                Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
                child = ch ? ch->prev_sibling : TETHER_INVALID_GUID;
            }
        }
        return TETHER_INVALID_GUID;
    }

    /* 2. Determine hit behavior from Interactable component, or default to child-only if non-interactable */
    Tether_HitBehavior hit_behavior = interact ? interact->hit_behavior : TETHER_HIT_CHILD_ONLY;

    if (hit_behavior == TETHER_HIT_IGNORE) {
        return TETHER_INVALID_GUID;
    }

    /* 3. Reverse DFS: Test children back-to-front (top-most Z-Index first), UNLESS we are SELF_ONLY */
    if (hit_behavior != TETHER_HIT_SELF_ONLY) {
        if (h && h->last_child != TETHER_INVALID_GUID) {
            Tether_GUID child = h->last_child;
            while (child != TETHER_INVALID_GUID) {
                Tether_GUID hit = hit_test_tree(child, x, y);
                if (hit != TETHER_INVALID_GUID) return hit;

                Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
                child = ch ? ch->prev_sibling : TETHER_INVALID_GUID;
            }
        }
    }

    /* 4. If children didn't catch it, do I catch it? */
    if (hit_behavior == TETHER_HIT_CHILD_ONLY) {
        return TETHER_INVALID_GUID;
    }

    return entity;
}

Tether_GUID tether_hit_test(float x, float y) {
    if (tether_ecs_is_valid(g_route)) {
        Tether_GUID hit = hit_test_tree(g_route, x, y);
        if (hit != TETHER_INVALID_GUID) return hit;
    }
    /* Traverse ALL root entities (same strategy as the layout engine) */
    Tether_DenseArray* hierarchies = tether_ecs_get_dense_array(TETHER_COMPONENT_HIERARCHY);
    if (!hierarchies) return TETHER_INVALID_GUID;

    /* Iterate backwards so overlay-like roots are tested first */
    for (int i = (int)hierarchies->count - 1; i >= 0; i--) {
        Tether_GUID entity = hierarchies->entity_map[i];
        if (!tether_ecs_is_valid(entity)) continue;

        Tether_Hierarchy* h = (Tether_Hierarchy*)((uint8_t*)hierarchies->data + (i * hierarchies->element_size));
        /* Only process root nodes (no parent) */
        if (!tether_ecs_is_valid(h->parent)) {
            Tether_GUID hit = hit_test_tree(entity, x, y);
            if (hit != TETHER_INVALID_GUID) return hit;
        }
    }
    return TETHER_INVALID_GUID;
}

void tether_input_process_event(const Tether_Pointer_Event* event) {
    if (!event) return;

    g_pointer_x = event->x;
    g_pointer_y = event->y;

    bool is_down = (event->type == TETHER_POINTER_DOWN);
    bool is_up = (event->type == TETHER_POINTER_UP);
    
    Tether_Event info = { .button = event->button, .x = event->x, .y = event->y };
    if (g_hovered != TETHER_INVALID_GUID) {
        Tether_Interactable *hovered = tether_ecs_get_component(g_hovered, TETHER_COMPONENT_INTERACTABLE);
        if (!hovered || !hovered->is_enabled || !hovered->is_hovered) g_hovered = TETHER_INVALID_GUID;
    }
    if (g_pressed != TETHER_INVALID_GUID) {
        Tether_Interactable *pressed = tether_ecs_get_component(g_pressed, TETHER_COMPONENT_INTERACTABLE);
        if (!pressed || !pressed->is_enabled || !pressed->is_pressed) g_pressed = TETHER_INVALID_GUID;
    }
    if (g_pressed != TETHER_INVALID_GUID && event->type == TETHER_POINTER_MOVE) {
        info.type = TETHER_EVENT_DRAG;
        info.button = TETHER_MOUSE_BUTTON_LEFT;
        tether_dispatch_event_data(g_pressed, &info);
        info.button = event->button;
    }
    
    Tether_GUID hit = tether_hit_test(event->x, event->y);
    if (!tether_ecs_is_valid(g_route)) {
        g_route = TETHER_INVALID_GUID;
        g_route_callback = NULL;
    } else if (g_route_callback) {
        g_route_callback(g_route, event, hit, g_pressed);
    }
    /* ========================================================================== */
    /*                            Hover Transitions                               */
    /* ========================================================================== */
    if (hit != g_hovered) {
        if (g_hovered != TETHER_INVALID_GUID) {
            Tether_Interactable* old_i = (Tether_Interactable*)tether_ecs_get_component(g_hovered, TETHER_COMPONENT_INTERACTABLE);
            if (old_i) {
                old_i->is_hovered = 0;
                tether_ecs_add_component(g_hovered, TETHER_COMPONENT_DIRTY_VISUAL);
                info.type = TETHER_EVENT_HOVER_EXIT;
                tether_dispatch_event_data(g_hovered, &info);
            }
        }

        if (hit != TETHER_INVALID_GUID) {
            Tether_Interactable* new_i = (Tether_Interactable*)tether_ecs_get_component(hit, TETHER_COMPONENT_INTERACTABLE);
            if (new_i) {
                new_i->is_hovered = 1;
                tether_ecs_add_component(hit, TETHER_COMPONENT_DIRTY_VISUAL);
                info.type = TETHER_EVENT_HOVER_ENTER;
                tether_dispatch_event_data(hit, &info);
            }
        }
        g_hovered = hit;
    }
    /* ========================================================================== */
    /*                       Press / Release Transitions                          */
    /* ========================================================================== */
    if (is_down && event->button == TETHER_MOUSE_BUTTON_LEFT) {
        if (g_pressed != hit) {
            if (g_pressed != TETHER_INVALID_GUID) {
                Tether_Interactable* old_p = (Tether_Interactable*)tether_ecs_get_component(g_pressed, TETHER_COMPONENT_INTERACTABLE);
                if (old_p) {
                    old_p->is_pressed = 0;
                    tether_ecs_add_component(g_pressed, TETHER_COMPONENT_DIRTY_VISUAL);
                }
            }
            g_pressed = hit;
            if (g_pressed != TETHER_INVALID_GUID) {
                Tether_Interactable* p = (Tether_Interactable*)tether_ecs_get_component(g_pressed, TETHER_COMPONENT_INTERACTABLE);
                if (p) {
                    p->is_pressed = 1;
                    tether_ecs_add_component(g_pressed, TETHER_COMPONENT_DIRTY_VISUAL);
                }
                info.type = TETHER_EVENT_PRESS;
                tether_dispatch_event_data(g_pressed, &info);
            }
        }
    } 
    else if (is_down && event->button == TETHER_MOUSE_BUTTON_RIGHT) {
        g_right_pressed = hit != TETHER_INVALID_GUID ? hit : tether_ecs_get_main_root();
        info.type = TETHER_EVENT_PRESS;
        tether_dispatch_event_data(g_right_pressed, &info);
    }
    else if (is_up && event->button == TETHER_MOUSE_BUTTON_LEFT) {
        if (g_pressed != TETHER_INVALID_GUID) {
            Tether_Interactable* p = (Tether_Interactable*)tether_ecs_get_component(g_pressed, TETHER_COMPONENT_INTERACTABLE);
            if (p) {
                p->is_pressed = 0;
                tether_ecs_add_component(g_pressed, TETHER_COMPONENT_DIRTY_VISUAL);
            }
            
            info.type = TETHER_EVENT_RELEASE;
            tether_dispatch_event_data(g_pressed, &info);

            /* If we released on the SAME entity we pressed, it's a click! */
            if (g_pressed == hit) {
                info.type = TETHER_EVENT_CLICK;
                tether_dispatch_event_data(g_pressed, &info);
            }
            g_pressed = TETHER_INVALID_GUID;
        }
    }
    else if (is_up && event->button == TETHER_MOUSE_BUTTON_RIGHT) {
        Tether_GUID target = hit != TETHER_INVALID_GUID ? hit : tether_ecs_get_main_root();
        Tether_GUID pressed = g_right_pressed;
        g_right_pressed = TETHER_INVALID_GUID;
        info.type = TETHER_EVENT_RELEASE;
        tether_dispatch_event_data(pressed, &info);
        if (pressed == target && tether_ecs_is_valid(pressed)) {
            info.type = TETHER_EVENT_CLICK;
            while (tether_ecs_is_valid(target)) {
                Tether_Hierarchy *hierarchy = tether_ecs_get_component(target, TETHER_COMPONENT_HIERARCHY);
                Tether_GUID parent = hierarchy ? hierarchy->parent : TETHER_INVALID_GUID;
                tether_dispatch_event_data(target, &info);
                target = parent;
            }
        }
    }
}

void tether_input_term(void) {
    g_hovered = TETHER_INVALID_GUID;
    g_pressed = TETHER_INVALID_GUID;
    g_route = TETHER_INVALID_GUID;
    g_route_callback = NULL;
    g_right_pressed = TETHER_INVALID_GUID;
    g_pointer_x = 0.0f;
    g_pointer_y = 0.0f;
}
