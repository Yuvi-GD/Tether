#ifndef TETHER_MENU_H
#define TETHER_MENU_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Visual floating menu panel widget.
 * Holds an interactive vertical list of items (buttons).
 * Uses tether_list under the hood for logical selection state.
 * Shared by tether_dropdown and context menus.
 */

Tether_GUID tether_ui_menu(Tether_GUID parent);

void tether_ui_menu_set_items(Tether_GUID menu, const char **items, int count);
void tether_ui_menu_set_selected(Tether_GUID menu, int selected_index);
int  tether_ui_menu_get_selected(Tether_GUID menu);
int  tether_ui_menu_get_item_count(Tether_GUID menu);
const char* tether_ui_menu_get_item_text(Tether_GUID menu, int index);

void tether_ui_menu_open(Tether_GUID menu);
void tether_ui_menu_close(Tether_GUID menu);
bool tether_ui_menu_is_open(Tether_GUID menu);

/* Position anchored relative to a header/trigger entity with top offset */
void tether_ui_menu_position_relative(Tether_GUID menu, Tether_GUID anchor_header, float offset_y);

/* Position anchored at explicit coordinates (e.g. for context menu) */
void tether_ui_menu_position_at(Tether_GUID menu, float x, float y, float width);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_MENU_H */
