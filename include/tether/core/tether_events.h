#ifndef TETHER_EVENTS_H
#define TETHER_EVENTS_H

#include "tether/core/tether_ecs.h"

typedef enum {
    TETHER_EVENT_HOVER_ENTER,
    TETHER_EVENT_HOVER_EXIT,
    TETHER_EVENT_PRESS,
    TETHER_EVENT_RELEASE,
    TETHER_EVENT_CLICK
} Tether_EventType;

typedef void (*Tether_EventCallback)(Tether_GUID entity, Tether_EventType type, void* user_data);

/* Bind a C-function to an entity for a specific event type */
void tether_bind_event(Tether_GUID entity, Tether_EventType type, Tether_EventCallback callback, void* user_data);

/* Remove a binding */
void tether_unbind_event(Tether_GUID entity, Tether_EventType type);

/* System function called by the input engine to dispatch events */
void tether_dispatch_event(Tether_GUID entity, Tether_EventType type);

/* Clear all bindings (called on shutdown) */
void tether_events_term(void);

#endif
