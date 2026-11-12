#include "tether/core/tether_events.h"

#include <stdlib.h>
#include <stdio.h>


typedef struct {
    Tether_GUID entity;
    Tether_EventType type;
    Tether_MouseButton button;
    Tether_EventCallback callback;
    void* user_data;
} Tether_EventBinding;

static Tether_EventBinding* g_bindings = NULL;
static size_t g_binding_count = 0;
static size_t g_binding_capacity = 0;
static unsigned g_dispatch_depth = 0;
static bool g_removed_bindings = false;

static bool is_pointer_action(Tether_EventType type) {
    return type == TETHER_EVENT_PRESS || type == TETHER_EVENT_RELEASE ||
           type == TETHER_EVENT_CLICK || type == TETHER_EVENT_DRAG;
}

static bool ensure_binding_capacity(void) {
    if (g_binding_count >= g_binding_capacity) {
        size_t capacity = g_binding_capacity == 0 ? 16 : g_binding_capacity * 2;
        Tether_EventBinding *bindings = realloc(g_bindings, capacity * sizeof(*bindings));
        if (!bindings) return false;
        g_bindings = bindings;
        g_binding_capacity = capacity;
    }
    return true;
}

static void remove_binding(size_t index) {
    if (g_dispatch_depth) {
        g_bindings[index].callback = NULL;
        g_removed_bindings = true;
    } else {
        g_bindings[index] = g_bindings[--g_binding_count];
    }
}

static void bind_event(Tether_GUID entity, Tether_EventType type, Tether_MouseButton button,
                       Tether_EventCallback callback, void *user_data) {
    if (!tether_ecs_is_valid(entity) || !callback) return;

    /* If this exact callback is already bound to this entity & type, update its user_data */
    for (size_t i = 0; i < g_binding_count; i++) {
        if (g_bindings[i].entity == entity && g_bindings[i].type == type &&
            g_bindings[i].button == button && g_bindings[i].callback == callback) {
            g_bindings[i].user_data = user_data;
            return;
        }
    }

    if (!ensure_binding_capacity()) return;

    /* Add new binding */
    g_bindings[g_binding_count].entity = entity;
    g_bindings[g_binding_count].type = type;
    g_bindings[g_binding_count].button = button;
    g_bindings[g_binding_count].callback = callback;
    g_bindings[g_binding_count].user_data = user_data;
    g_binding_count++;
}

void tether_bind_event(Tether_GUID entity, Tether_EventType type, Tether_EventCallback callback, void *user_data) {
    bind_event(entity, type, TETHER_MOUSE_BUTTON_LEFT, callback, user_data);
}

void tether_bind_pointer_event(Tether_GUID entity, Tether_EventType type, Tether_MouseButton button,
                               Tether_EventCallback callback, void *user_data) {
    if (!is_pointer_action(type) || button < TETHER_MOUSE_BUTTON_LEFT || button > TETHER_MOUSE_BUTTON_MIDDLE) return;
    bind_event(entity, type, button, callback, user_data);
}

void tether_unbind_callback(Tether_GUID entity, Tether_EventType type, Tether_EventCallback callback) {
    for (size_t i = 0; i < g_binding_count; ) {
        if (g_bindings[i].entity == entity && g_bindings[i].type == type && g_bindings[i].callback == callback) {
            /* Swap and pop */
            remove_binding(i);
            if (g_dispatch_depth) i++;
        } else i++;
    }
}

void tether_unbind_event(Tether_GUID entity, Tether_EventType type) {
    for (size_t i = 0; i < g_binding_count; ) {
        if (g_bindings[i].entity == entity && g_bindings[i].type == type) {
            remove_binding(i);
            if (g_dispatch_depth) i++;
        } else {
            i++;
        }
    }
}

void tether_clear_bindings(Tether_GUID entity) {
    for (size_t i = 0; i < g_binding_count; ) {
        if (g_bindings[i].entity == entity) {
            remove_binding(i);
            if (g_dispatch_depth) i++;
        } else {
            i++;
        }
    }
}

bool tether_has_event(Tether_GUID entity, Tether_EventType type) {
    if (!tether_ecs_is_valid(entity)) return false;
    for (size_t i = 0; i < g_binding_count; i++) {
        if (g_bindings[i].callback && g_bindings[i].entity == entity && g_bindings[i].type == type) {
            return true;
        }
    }
    return false;
}

void tether_dispatch_event(Tether_GUID entity, Tether_EventType type) {
    Tether_Event event = { .type = type,
        .button = is_pointer_action(type) ? TETHER_MOUSE_BUTTON_LEFT : TETHER_MOUSE_BUTTON_NONE };
    tether_dispatch_event_data(entity, &event);
}

void tether_dispatch_event_data(Tether_GUID entity, const Tether_Event *event) {
    if (!event || !tether_ecs_is_valid(entity)) return;
    size_t limit = g_binding_count;
    g_dispatch_depth++;
    for (size_t i = 0; i < limit && tether_ecs_is_valid(entity); i++) {
        Tether_EventBinding binding = g_bindings[i];
        if (binding.callback && binding.entity == entity && binding.type == event->type &&
            (!is_pointer_action(event->type) || binding.button == event->button)) {
            binding.callback(entity, event, binding.user_data);
        }
    }
    if (--g_dispatch_depth == 0 && g_removed_bindings) {
        for (size_t i = 0; i < g_binding_count; ) {
            if (!g_bindings[i].callback) remove_binding(i);
            else i++;
        }
        g_removed_bindings = false;
    }
}

void tether_events_term(void) {
    if (g_bindings) {
        free(g_bindings);
        g_bindings = NULL;
    }
    g_binding_count = 0;
    g_binding_capacity = 0;
    g_removed_bindings = false;
}
