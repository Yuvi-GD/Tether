#ifndef SANDBOX_UI_H
#define SANDBOX_UI_H

#include "tether/ui/tether_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Theme Colors for Sandbox Workbench */
#define SANDBOX_COLOR_BG          ((Tether_Color){18, 22, 31, 255})
#define SANDBOX_COLOR_PANEL       ((Tether_Color){26, 34, 48, 255})
#define SANDBOX_COLOR_BORDER      ((Tether_Color){42, 54, 74, 255})
#define SANDBOX_COLOR_HEADER_TEXT ((Tether_Color){225, 233, 246, 255})
#define SANDBOX_COLOR_SUB_TEXT    ((Tether_Color){155, 173, 198, 255})
#define SANDBOX_COLOR_MUTED_TEXT  ((Tether_Color){135, 155, 185, 255})
#define SANDBOX_COLOR_CONSOLE_BG  ((Tether_Color){13, 16, 23, 255})
#define SANDBOX_COLOR_CONSOLE_BD  ((Tether_Color){35, 45, 62, 255})

/* Status Colors */
#define SANDBOX_COLOR_PASSED      ((Tether_Color){112, 222, 166, 255})
#define SANDBOX_COLOR_FAILED      ((Tether_Color){255, 130, 140, 255})
#define SANDBOX_COLOR_WARNING     ((Tether_Color){245, 195, 115, 255})
#define SANDBOX_COLOR_READY       ((Tether_Color){150, 195, 235, 255})

/* Create a numbered/accented section container column */
Tether_GUID sandbox_ui_section(Tether_GUID parent, const char *title);

/* Create a standard workbench header (Title + Subtitle) */
Tether_GUID sandbox_ui_header(Tether_GUID parent, const char *title, const char *subtitle);

/* Create a styled execution report console box with dark background and tag */
Tether_GUID sandbox_ui_console_box(Tether_GUID parent, const char *tag_title, float wrap_width, Tether_GUID *out_text_entity);

/* Create a standard sandbox button (height 48, flex 1, rounded corners, centered text, auto hover/press).
 * Returns the text entity so caller can directly access/update text properties. */
Tether_GUID sandbox_ui_button(Tether_GUID parent, const char *caption, Tether_EventCallback callback, void *user_data);

Tether_GUID sandbox_ui_line(Tether_GUID parent, float thickness, Tether_Color color);

#ifdef __cplusplus
}
#endif

#endif /* SANDBOX_UI_H */
