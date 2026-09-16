#ifndef TETHER_INPUT_H
#define TETHER_INPUT_H

#include "tether/core/tether_ecs.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TETHER_POINTER_DOWN = 0,
    TETHER_POINTER_UP,
    TETHER_POINTER_MOVE
} Tether_Pointer_EventType;

typedef enum {
    TETHER_MOUSE_BUTTON_LEFT = 0,
    TETHER_MOUSE_BUTTON_RIGHT,
    TETHER_MOUSE_BUTTON_MIDDLE,
    TETHER_MOUSE_BUTTON_NONE
} Tether_MouseButton;

typedef struct Tether_Pointer_Event {
    Tether_Pointer_EventType type;
    Tether_MouseButton button;
    float x;
    float y;
} Tether_Pointer_Event;

/* Perform a Z-Index accurate spatial query to find the topmost entity at (x,y).
 * Culls layout branches instantly if the point is outside the parent's SlotTransform.
 * Respects Tether_HitBehavior properties.
 */
Tether_GUID tether_hit_test(float x, float y);

/*
 * Process a normalized input event.
 * Handles state transitions (Hover Enter/Exit, Press, Release, Click) and
 * automatically fires the Tether_EventRegistry callbacks.
 */
void tether_input_process_event(Tether_Pointer_Event* event);

/* Reset input tracking (called on shutdown) */
void tether_input_term(void);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_INPUT_H */
