#include "tether/core/tether_events.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    Tether_GUID entity;
    Tether_EventType type;
    Tether_EventCallback callback;
    void* user_data;
} Tether_EventBinding;

static Tether_EventBinding* g_bindings = NULL;
static size_t g_binding_count = 0;
static size_t g_binding_capacity = 0;

void tether_bind_event(Tether_GUID entity, Tether_EventType type, Tether_EventCallback callback, void* user_data) {
    if (!tether_ecs_is_valid(entity) || !callback) return;

    /* Replace existing binding if it exists */
    for (size_t i = 0; i < g_binding_count; i++) {
        if (g_bindings[i].entity == entity && g_bindings[i].type == type) {
            g_bindings[i].callback = callback;
            g_bindings[i].user_data = user_data;
            return;
        }
    }

    /* Grow array if needed */
    if (g_binding_count >= g_binding_capacity) {
        g_binding_capacity = g_binding_capacity == 0 ? 16 : g_binding_capacity * 2;
        g_bindings = (Tether_EventBinding*)realloc(g_bindings, g_binding_capacity * sizeof(Tether_EventBinding));
    }

    /* Add new binding */
    g_bindings[g_binding_count].entity = entity;
    g_bindings[g_binding_count].type = type;
    g_bindings[g_binding_count].callback = callback;
    g_bindings[g_binding_count].user_data = user_data;
    g_binding_count++;
}

void tether_unbind_event(Tether_GUID entity, Tether_EventType type) {
    for (size_t i = 0; i < g_binding_count; i++) {
        if (g_bindings[i].entity == entity && g_bindings[i].type == type) {
            /* Swap and pop */
            g_bindings[i] = g_bindings[g_binding_count - 1];
            g_binding_count--;
            return;
        }
    }
}

void tether_dispatch_event(Tether_GUID entity, Tether_EventType type) {
    if (!tether_ecs_is_valid(entity)) return;

    for (size_t i = 0; i < g_binding_count; i++) {
        if (g_bindings[i].entity == entity && g_bindings[i].type == type) {
            g_bindings[i].callback(entity, type, g_bindings[i].user_data);
        }
    }
}

void tether_events_term(void) {
    if (g_bindings) {
        free(g_bindings);
        g_bindings = NULL;
    }
    g_binding_count = 0;
    g_binding_capacity = 0;
}
