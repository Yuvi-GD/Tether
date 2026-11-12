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
Tether_GUID tether_ui_segmented_bar(Tether_GUID parent);

/* Change segment direction after creation; only ROW and COLUMN are supported. */
void tether_ui_set_segmented_bar_flow(Tether_GUID bar, Tether_Flow flow);

/* ========================================================================== */
/* 2. Operations (Setters, Getters, Item Management)                          */
/* ========================================================================== */

/* Dynamic Item Management */
Tether_GUID tether_ui_add_segment(Tether_GUID bar, const char *label);
void        tether_ui_set_segment(Tether_GUID bar, int index, const char *label);
const char* tether_ui_get_segment(Tether_GUID bar, int index);
void        tether_ui_remove_segment(Tether_GUID bar, int index);
void        tether_ui_clear_segments(Tether_GUID bar);

/* Selection Operations */
void        tether_ui_set_segment_selected(Tether_GUID bar, int selected_index);
int         tether_ui_get_segment_selected(Tether_GUID bar);
void        tether_ui_unselect_segment(Tether_GUID bar);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_SEGMENTED_BAR_H */
