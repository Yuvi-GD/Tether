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
/* Containers & Flex Layout                                                   */
/* ========================================================================== */

/* Create a styled container panel */
Tether_GUID tether_ui_panel(Tether_GUID parent, Tether_Color bg_color, float radius);

/* Create a horizontal flex row container */
Tether_GUID tether_ui_row(Tether_GUID parent, float gap_x);

/* Create a vertical flex column container */
Tether_GUID tether_ui_column(Tether_GUID parent, float gap_y);

/* ========================================================================== */
/* Content (Unified Text)                                                     */
/* ========================================================================== */

/* Create a styled text element */
Tether_GUID tether_ui_text(Tether_GUID parent, const char *string, float font_size, Tether_Color color);

/* Update text string on an entity (or its text child), auto-flagging dirty flags */
bool tether_ui_set_text(Tether_GUID entity, const char *string);

/* Get current text string from an entity (or its text child) */
const char* tether_ui_get_text(Tether_GUID entity);

/* ========================================================================== */
/* Interactive Controls                                                       */
/* ========================================================================== */

/* Create an interactive button with caption text, auto-hover/press states, and click callback */
Tether_GUID tether_ui_button(Tether_GUID parent, const char *caption, float font_size,
                             Tether_Color bg_color, Tether_EventCallback callback, void *user_data);

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

/* Set background color of an entity's shape */
void tether_ui_set_color(Tether_GUID entity, Tether_Color bg_color);

/* Set border width, color, and corner radius */
void tether_ui_set_border(Tether_GUID entity, float width, Tether_Color color, float radius);

/* Set text color on an entity (or its text child) */
void tether_ui_set_text_color(Tether_GUID entity, Tether_Color color);

/* Set text alignment on an entity (or its text child) */
void tether_ui_set_text_align(Tether_GUID entity, Tether_Align align_x, Tether_Align align_y);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_UI_H */
