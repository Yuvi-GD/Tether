#ifndef TETHER_CHECKBOX_H
#define TETHER_CHECKBOX_H

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
 * Hierarchy (two entities):
 * [Self] Box: Shape 20x20, BoolState, Interactable
 *   └── [0] Indicator: Shape 15x15
 * Place a separate Text widget alongside the checkbox when a label is needed.
 */
Tether_GUID tether_ui_checkbox(Tether_GUID parent);

/* ========================================================================== */
/* 2. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void tether_ui_set_checked(Tether_GUID checkbox, bool checked);
bool tether_ui_get_checked(Tether_GUID checkbox);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_CHECKBOX_H */
