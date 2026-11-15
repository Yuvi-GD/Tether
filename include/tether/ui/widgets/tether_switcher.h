#ifndef TETHER_SWITCHER_H
#define TETHER_SWITCHER_H

#include "tether/core/tether_ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

/*
 * Hierarchy Tree:
 * [Self] Container (column, flex=1.0), SelectionState
 *   ├── [0] View 0 (visible)
 *   └── [N-1] View N-1 (collapsed)
 */
TUID tether_ui_switcher(TUID parent);

/* ========================================================================== */
/* 2. Operations (View Management & Activation)                               */
/* ========================================================================== */

TUID tether_ui_switcher_add_view(TUID switcher);
void        tether_ui_switcher_show_index(TUID switcher, int active_index);
int         tether_ui_switcher_get_active(TUID switcher);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_SWITCHER_H */
