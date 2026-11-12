#ifndef TETHER_SLIDER_H
#define TETHER_SLIDER_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"
#include "tether/ui/tether_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. Widget Creation                                                         */
/* ========================================================================== */

/*
 * Hierarchy Tree (total entities: 3):
 * [Self] Track (from progress_bar): Panel (Flow None, flex=1.0), ValueRange, Interactable
 *   ├── [0] Fill: Panel (active bar)
 *   └── [1] Thumb: Panel 18x18
 */
Tether_GUID tether_ui_slider(Tether_GUID parent);

/* ========================================================================== */
/* 2. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void tether_ui_set_slider_value(Tether_GUID slider, float value);
float tether_ui_get_slider_value(Tether_GUID slider);
void tether_ui_set_slider_range(Tether_GUID slider, float min_val, float max_val);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_SLIDER_H */
