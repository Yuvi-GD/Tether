#ifndef TETHER_COLLAPSIBLE_H
#define TETHER_COLLAPSIBLE_H

#include "tether/core/tether_ecs.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

/*
 * Hierarchy Tree (total entities: 4 + children of content):
 * [Self] Container (outer column), BoolState
 *   ├── [0] Header: Panel (row), Interactable
 *   │     ├── [0] Arrow: Text (">" / "v")
 *   │     └── [1] Title: Text
 *   └── [1] Content: Container (column)
 */
TUID tether_ui_collapsible(TUID parent);

/* ========================================================================== */
/* 2. Operations (Setters & Getters)                                          */
/* ========================================================================== */

TUID tether_ui_collapsible_content(TUID collapsible);
void        tether_ui_toggle_collapsible(TUID collapsible);
bool        tether_ui_is_collapsible_open(TUID collapsible);
void        tether_ui_set_collapsible_title(TUID collapsible, const char *title);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_COLLAPSIBLE_H */
