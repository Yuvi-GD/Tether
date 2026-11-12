#ifndef TETHER_BUTTON_H
#define TETHER_BUTTON_H

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
 * Hierarchy Tree (total entities: 2):
 * [Self] Panel (button), Layout, Interactable
 *   └── [0] Label: Text ("Button")
 */
Tether_GUID tether_ui_button(Tether_GUID parent);

/* ========================================================================== */
/* 2. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void tether_ui_set_button_label(Tether_GUID btn, const char* label);
const char* tether_ui_get_button_label(Tether_GUID btn);
void tether_ui_set_button_color(Tether_GUID btn, Tether_Color color);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_BUTTON_H */
