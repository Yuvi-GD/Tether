#ifndef TETHER_TOGGLE_H
#define TETHER_TOGGLE_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

/*
 * Hierarchy Tree (total entities: 2):
 * [Self] Panel 46x26, BoolState, Interactable
 *   └── [0] Thumb: Panel 20x20
 */
TUID tether_ui_toggle(TUID parent);

/* ========================================================================== */
/* 2. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void tether_ui_set_toggle(TUID toggle, bool is_on);
bool tether_ui_get_toggle(TUID toggle);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_TOGGLE_H */
