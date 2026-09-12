#ifndef TETHER_INPUT_H
#define TETHER_INPUT_H

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

/*
 * Process a normalized input event.
 * In a full system, this would perform spatial intersection against the ECS Transform array.
 */
void tether_input_process_event(Tether_Pointer_Event* event);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_INPUT_H */
