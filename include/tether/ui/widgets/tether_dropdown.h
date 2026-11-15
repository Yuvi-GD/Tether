#ifndef TETHER_DROPDOWN_H
#define TETHER_DROPDOWN_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. Types & Creation                                                        */
/* ========================================================================== */

typedef enum {
    TETHER_DROPDOWN_INLINE = 0,
    TETHER_DROPDOWN_OVERLAY
} Tether_DropdownType;

/*
 * Hierarchy Tree (Inline: total entities: 3 + 2*N, Overlay: list detached to overlay root):
 * [Self] Container (outer column), SelectionState
 *   ├── [0] Header: Panel (row), Interactable
 *   │     ├── [0] Header Text: Text
 *   │     └── [1] Arrow Text: Text ("v" / "^")
 *   └── [1] List Panel: Container / Overlay
 *         └── [i] Item: Panel, Interactable
 *               └── [0] Text
 */
TUID tether_ui_dropdown(TUID parent);

/* ========================================================================== */
/* 2. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void tether_ui_set_dropdown_selected(TUID dropdown, int selected_index);
int  tether_ui_get_dropdown_selected(TUID dropdown);
void tether_ui_set_dropdown_open(TUID dropdown, bool open);
bool tether_ui_is_dropdown_open(TUID dropdown);
void tether_ui_set_dropdown_options(TUID dropdown, const char **options, int count);
void tether_ui_set_dropdown_type(TUID dropdown, Tether_DropdownType type);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_DROPDOWN_H */
