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
Tether_GUID tether_ui_switcher(Tether_GUID parent);

/* ========================================================================== */
/* 2. Operations (View Management & Activation)                               */
/* ========================================================================== */

Tether_GUID tether_ui_switcher_add_view(Tether_GUID switcher);
void        tether_ui_switcher_show_index(Tether_GUID switcher, int active_index);
int         tether_ui_switcher_get_active(Tether_GUID switcher);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_SWITCHER_H */
