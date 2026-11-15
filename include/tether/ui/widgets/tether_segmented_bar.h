#ifndef TETHER_SEGMENTED_BAR_H
#define TETHER_SEGMENTED_BAR_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_events.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

/*
 * Hierarchy Tree (total entities: 1 + 2*N):
 * [Self] Panel (bar), SelectionState
 *   ├── [0] Segment 0: Shape, Interactable
 *   │     └── [0] Text
 *   └── [N-1] Segment N-1: Shape, Interactable
 *         └── [0] Text
 */
TUID tether_ui_segmented_bar(TUID parent);

/* Change segment direction after creation; only ROW and COLUMN are supported. */
void tether_ui_set_segmented_bar_flow(TUID bar, Tether_Flow flow);

/* ========================================================================== */
/* 2. Operations (Setters, Getters, Item Management)                          */
/* ========================================================================== */

/* Dynamic Item Management */
TUID tether_ui_add_segment(TUID bar, const char *label);
void        tether_ui_set_segment(TUID bar, int index, const char *label);
const char* tether_ui_get_segment(TUID bar, int index);
void        tether_ui_remove_segment(TUID bar, int index);
void        tether_ui_clear_segments(TUID bar);

/* Selection Operations */
void        tether_ui_set_segment_selected(TUID bar, int selected_index);
int         tether_ui_get_segment_selected(TUID bar);
void        tether_ui_unselect_segment(TUID bar);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_SEGMENTED_BAR_H */
