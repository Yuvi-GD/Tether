#ifndef TETHER_EVENTS_H
#define TETHER_EVENTS_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_input.h"

typedef enum {
    TETHER_EVENT_HOVER_ENTER,
    TETHER_EVENT_HOVER_EXIT,
    TETHER_EVENT_PRESS,
    TETHER_EVENT_RELEASE,
    TETHER_EVENT_CLICK,
    TETHER_EVENT_DRAG,
    TETHER_EVENT_CHANGE
} Tether_EventType;

/* Event data is borrowed and valid only for the duration of the callback. */
typedef struct {
    Tether_EventType type;
    Tether_MouseButton button;
    float x, y;
} Tether_Event;

typedef void (*Tether_EventCallback)(Tether_GUID entity, const Tether_Event *event, void* user_data);

/* Pointer actions default to the left button; hover and CHANGE are unfiltered.
 * CHANGE means observable state changed: read the entity's current state.
 * user_data is borrowed from the caller and is never freed by Tether. */
void tether_bind_event(Tether_GUID entity, Tether_EventType type, Tether_EventCallback callback, void* user_data);
void tether_bind_pointer_event(Tether_GUID entity, Tether_EventType type, Tether_MouseButton button,
                               Tether_EventCallback callback, void *user_data);

/* Remove all bindings of a specific event type on an entity */
void tether_unbind_event(Tether_GUID entity, Tether_EventType type);

/* Remove a specific callback binding on an entity */
void tether_unbind_callback(Tether_GUID entity, Tether_EventType type, Tether_EventCallback callback);

/* Remove all event bindings on an entity (called on entity destruction) */
void tether_clear_bindings(Tether_GUID entity);

/* During dispatch, removed callbacks are skipped; newly bound callbacks run on
 * the next dispatch. Destroying the entity stops delivery to that entity. */
void tether_dispatch_event(Tether_GUID entity, Tether_EventType type);
void tether_dispatch_event_data(Tether_GUID entity, const Tether_Event *event);

/* Check if an entity has any bindings for a specific event type */
bool tether_has_event(Tether_GUID entity, Tether_EventType type);

/* Clear all bindings (called on shutdown) */
void tether_events_term(void);

#endif
