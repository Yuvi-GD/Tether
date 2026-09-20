#include "tether/core/tether_input.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_events.h"
#include <stdlib.h>
#include <stdio.h>

static Tether_GUID g_hovered = TETHER_INVALID_GUID;
static Tether_GUID g_pressed = TETHER_INVALID_GUID;

static Tether_GUID hit_test_tree(Tether_GUID entity, float x, float y) {
    if (!tether_ecs_is_valid(entity)) return TETHER_INVALID_GUID;

    Tether_LayoutNode* node = (Tether_LayoutNode*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT_NODE);
    Tether_Visibility* vis = (Tether_Visibility*)tether_ecs_get_component(entity, TETHER_COMPONENT_VISIBILITY);
    if (vis && (vis->state == TETHER_HIDDEN || vis->state == TETHER_COLLAPSED)) {
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

    /* 1. O(1) Spatial Culling: If mouse is outside this node, skip entire branch */
    if (x < st->x || x > st->x + st->width || y < st->y || y > st->y + st->height) {
        return TETHER_INVALID_GUID;
    }

    /* 2. Check hit_behavior cull */
    if (node->hit_behavior == TETHER_HIT_IGNORE_ALL) {
        return TETHER_INVALID_GUID;
    }

    /* 3. Reverse DFS: Test children back-to-front (top-most Z-Index first) */
    if (h && h->last_child != TETHER_INVALID_GUID) {
        Tether_GUID child = h->last_child;
        while (child != TETHER_INVALID_GUID) {
            Tether_GUID hit = hit_test_tree(child, x, y);
            if (hit != TETHER_INVALID_GUID) return hit;

            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            child = ch ? ch->prev_sibling : TETHER_INVALID_GUID;
        }
    }

    /* 4. If children didn't catch it, do I catch it? */
    if (node->hit_behavior == TETHER_HIT_IGNORE_SELF) {
        return TETHER_INVALID_GUID;
    }

    return entity;
}

Tether_GUID tether_hit_test(float x, float y) {
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

void tether_input_process_event(Tether_Pointer_Event* event) {
    if (!event) return;

    bool is_down = (event->type == TETHER_POINTER_DOWN);
    bool is_up = (event->type == TETHER_POINTER_UP);
    
    Tether_GUID hit = tether_hit_test(event->x, event->y);

    /* --- Hover Transitions --- */
    if (hit != g_hovered) {
        if (g_hovered != TETHER_INVALID_GUID) {
            Tether_Interactable* old_i = (Tether_Interactable*)tether_ecs_get_component(g_hovered, TETHER_COMPONENT_INTERACTABLE);
            if (old_i) {
                old_i->is_hovered = 0;
                tether_dispatch_event(g_hovered, TETHER_EVENT_HOVER_EXIT);
            }
        }

        if (hit != TETHER_INVALID_GUID) {
            Tether_Interactable* new_i = (Tether_Interactable*)tether_ecs_get_component(hit, TETHER_COMPONENT_INTERACTABLE);
            if (new_i) {
                new_i->is_hovered = 1;
                tether_dispatch_event(hit, TETHER_EVENT_HOVER_ENTER);
            }
        }
        g_hovered = hit;
    }

    /* --- Press / Release Transitions --- */
    if (is_down && event->button == TETHER_MOUSE_BUTTON_LEFT) {
        if (g_pressed != hit) {
            if (g_pressed != TETHER_INVALID_GUID) {
                Tether_Interactable* old_p = (Tether_Interactable*)tether_ecs_get_component(g_pressed, TETHER_COMPONENT_INTERACTABLE);
                if (old_p) old_p->is_pressed = 0;
            }
            g_pressed = hit;
            if (g_pressed != TETHER_INVALID_GUID) {
                Tether_Interactable* p = (Tether_Interactable*)tether_ecs_get_component(g_pressed, TETHER_COMPONENT_INTERACTABLE);
                if (p) p->is_pressed = 1;
                tether_dispatch_event(g_pressed, TETHER_EVENT_PRESS);
            }
        }
    } 
    else if (is_up && event->button == TETHER_MOUSE_BUTTON_LEFT) {
        if (g_pressed != TETHER_INVALID_GUID) {
            Tether_Interactable* p = (Tether_Interactable*)tether_ecs_get_component(g_pressed, TETHER_COMPONENT_INTERACTABLE);
            if (p) p->is_pressed = 0;
            
            tether_dispatch_event(g_pressed, TETHER_EVENT_RELEASE);

            /* If we released on the SAME entity we pressed, it's a click! */
            if (g_pressed == hit) {
                tether_dispatch_event(g_pressed, TETHER_EVENT_CLICK);
            }
            g_pressed = TETHER_INVALID_GUID;
        }
    }
}

void tether_input_term(void) {
    g_hovered = TETHER_INVALID_GUID;
    g_pressed = TETHER_INVALID_GUID;
}
