#ifndef TETHER_CONTEXT_MENU_H
#define TETHER_CONTEXT_MENU_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Create a menu widget under parent. Configure it through the menu API. */
Tether_GUID tether_ui_context_menu(Tether_GUID parent);

/*
 * Open a configured menu at logical screen coordinates. Parent it under a
 * viewport-sized container in the overlay root for screen-relative placement.
 * Bind TETHER_EVENT_CHANGE on the menu to receive selection changes.
 */
void tether_ui_context_menu_open_at(Tether_GUID menu, float x, float y);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_CONTEXT_MENU_H */
