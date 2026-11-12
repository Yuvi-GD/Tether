#ifndef TETHER_RADIO_H
#define TETHER_RADIO_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_events.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

/*
 * Hierarchy Tree (total entities: 1 + 4*N):
 * [Self] Container (radio group), SelectionState
 *   ├── [0] Option 0: Container (row), Interactable
 *   │     ├── [0] Ring: Shape 20x20
 *   │     │     └── [0] Dot: Shape 10x10
 *   │     └── [1] Label: Text
 *   └── [N-1] Option N-1: Container (row), Interactable
 *         ├── [0] Ring: Shape 20x20
 *         │     └── [0] Dot: Shape 10x10
 *         └── [1] Label: Text
 */
Tether_GUID tether_ui_radio(Tether_GUID parent);

/* Change the option direction after creation; only ROW and COLUMN are supported. */
void tether_ui_set_radio_flow(Tether_GUID radio, Tether_Flow flow);

/* ========================================================================== */
/* 2. Operations (Setters, Getters, Item Management)                          */
/* ========================================================================== */

/* Dynamic Item Management */
Tether_GUID tether_ui_add_radio_option(Tether_GUID radio, const char *label);
void        tether_ui_set_radio_option(Tether_GUID radio, int index, const char *label);
const char* tether_ui_get_radio_option(Tether_GUID radio, int index);
void        tether_ui_remove_radio_option(Tether_GUID radio, int index);
void        tether_ui_clear_radio_options(Tether_GUID radio);

/* Selection Operations */
void        tether_ui_set_radio_selected(Tether_GUID radio, int selected_index);
int         tether_ui_get_radio_selected(Tether_GUID radio);
void        tether_ui_unselect_radio(Tether_GUID radio);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_RADIO_H */
