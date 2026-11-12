#ifndef TETHER_PROGRESS_BAR_H
#define TETHER_PROGRESS_BAR_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
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
Tether_GUID tether_ui_progress_bar(Tether_GUID parent);

/* ========================================================================== */
/* 2. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void  tether_ui_set_progress(Tether_GUID progress_bar, float progress);
float tether_ui_get_progress(Tether_GUID progress_bar);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_PROGRESS_BAR_H */
