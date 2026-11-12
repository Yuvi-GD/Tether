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
Tether_GUID tether_ui_collapsible(Tether_GUID parent);

/* ========================================================================== */
/* 2. Operations (Setters & Getters)                                          */
/* ========================================================================== */

Tether_GUID tether_ui_collapsible_content(Tether_GUID collapsible);
void        tether_ui_toggle_collapsible(Tether_GUID collapsible);
bool        tether_ui_is_collapsible_open(Tether_GUID collapsible);
void        tether_ui_set_collapsible_title(Tether_GUID collapsible, const char *title);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_COLLAPSIBLE_H */
