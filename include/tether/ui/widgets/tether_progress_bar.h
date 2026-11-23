#ifndef TETHER_PROGRESS_BAR_H
#define TETHER_PROGRESS_BAR_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_events.h"
#include "tether/ui/tether_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. Widget Creation                                                         */
/* ========================================================================== */

/*
 * Hierarchy Tree (total entities: 2):
 * [Self] Track: Panel (Flow None, flex=1.0), ValueRange
 *   └── [0] Fill: Panel (active progress bar)
 */
TUID tether_ui_progress_bar(TUID parent);

/* ========================================================================== */
/* 2. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void  tether_ui_set_progress(TUID progress_bar, float progress);
float tether_ui_get_progress(TUID progress_bar);

void tether_ui_update_progress(TUID progress_bar);
void tether_ui_set_progress_value(TUID progress_bar, float value);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_PROGRESS_BAR_H */
