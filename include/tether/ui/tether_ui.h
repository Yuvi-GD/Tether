#ifndef TETHER_UI_H
#define TETHER_UI_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_events.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Primitives & Containers                                                    */
/* ========================================================================== */

/* Create a visual panel with shape (bg_color, border, radius) */
Tether_GUID tether_ui_panel(Tether_GUID parent);

/* Container Layout Configuration */
void tether_ui_set_container(Tether_GUID entity, Tether_Flow flow);
void tether_ui_set_position(Tether_GUID entity, Tether_PositionMode position);
Tether_PositionMode tether_ui_get_position(Tether_GUID entity);
void tether_ui_set_content_align(Tether_GUID entity, Tether_Align align_x, Tether_Align align_y);
void tether_ui_set_interactable(Tether_GUID entity, Tether_HitBehavior behavior);
Tether_HitBehavior tether_ui_get_interactable(Tether_GUID entity);

/* ========================================================================== */
/* Boolean State Operations                                                   */
/* ========================================================================== */

/* Checks if an entity has a boolean state component */
bool tether_ui_has_bool(Tether_GUID entity);

/* Reads the boolean state from an entity (returns false if missing) */
bool tether_ui_get_bool(Tether_GUID entity);

/* Sets the boolean state on an entity (returns false if missing) and flags dirty */
bool tether_ui_set_bool(Tether_GUID entity, bool value);

/* Toggles the boolean state on an entity and returns the new value */
bool tether_ui_toggle_bool(Tether_GUID entity);

/* ========================================================================== */
/* Value Range Operations                                                     */
/* ========================================================================== */

/* Reads the scalar value from an entity (returns 0.0f if missing) */
float tether_ui_get_value(Tether_GUID entity);

/* Sets the scalar value (clamped between min and max) and marks dirty */
bool  tether_ui_set_value(Tether_GUID entity, float value);

/* Configures the min and max bounds of the value range */
void  tether_ui_set_range(Tether_GUID entity, float min_val, float max_val);

/* Reads the min and max bounds */
void  tether_ui_get_range(Tether_GUID entity, float *out_min, float *out_max);

/* ========================================================================== */
/* Fluent Layout & Style Modifiers                                            */
/* ========================================================================== */

/* Set size box dimensions (width, height). 0 means auto/shrink-wrap */
void tether_ui_set_size(Tether_GUID entity, float width, float height);

/* Set container inner padding */
void tether_ui_set_padding(Tether_GUID entity, float top, float right, float bottom, float left);

/* Set gap between children */
void tether_ui_set_gap(Tether_GUID entity, float gap_x, float gap_y);

/* Set flex fill ratio along the parent container's primary axis */
void tether_ui_set_flex(Tether_GUID entity, float fill_ratio);

/* Set normalized anchor bounds [0..1] */
void tether_ui_set_anchor(Tether_GUID entity, float min_x, float min_y, float max_x, float max_y);

/* Set anchor pixel offsets (margins from anchor bounds) */
void tether_ui_set_offsets(Tether_GUID entity, float top, float right, float bottom, float left);

/* Set background color of an entity's shape */
void tether_ui_set_color(Tether_GUID entity, Tether_Color bg_color);

/* Get background color of an entity's shape */
Tether_Color tether_ui_get_color(Tether_GUID entity);

/* Adjust RGB channels by a signed amount, clamping to [0, 255]. Alpha is unchanged. */
Tether_Color tether_ui_adjust_color(Tether_Color color, int offset);

/* Interaction feedback: disabled/idle 0, pressed -40, hovered/focused +20. */
int tether_ui_interaction_offset(Tether_GUID entity);

/* Set border width, color, and corner radius */
void tether_ui_set_border(Tether_GUID entity, float width, Tether_Color color, float radius);

/* Active color modifiers (temporary state for hover/press while preserving base color) */
void tether_ui_set_active_color(Tether_GUID entity, Tether_Color color);
void tether_ui_reset_active_color(Tether_GUID entity);
void tether_ui_set_active_border(Tether_GUID entity, Tether_Color color);
void tether_ui_reset_active_border(Tether_GUID entity);

/* ========================================================================== */
/* Extended Controls & Component Primitives                                  */
/* (Implemented as standalone modules inside src/ui/widgets/)                 */
/* ========================================================================== */

#include "tether/ui/widgets/tether_button.h"
#include "tether/ui/widgets/tether_text.h"
#include "tether/ui/widgets/tether_progress_bar.h"
#include "tether/ui/widgets/tether_slider.h"
#include "tether/ui/widgets/tether_toggle.h"
#include "tether/ui/widgets/tether_checkbox.h"
#include "tether/ui/widgets/tether_radio.h"
#include "tether/ui/widgets/tether_segmented_bar.h"
#include "tether/ui/widgets/tether_menu.h"
#include "tether/ui/widgets/tether_dropdown.h"
#include "tether/ui/widgets/tether_context_menu.h"
#include "tether/ui/widgets/tether_collapsible.h"
#include "tether/ui/widgets/tether_switcher.h"

#ifdef __cplusplus
}
#endif

#endif /* TETHER_UI_H */
