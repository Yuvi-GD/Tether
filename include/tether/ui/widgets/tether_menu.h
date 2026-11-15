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

TUID tether_ui_menu(TUID parent);

void tether_ui_menu_set_items(TUID menu, const char **items, int count);
void tether_ui_menu_set_selected(TUID menu, int selected_index);
int  tether_ui_menu_get_selected(TUID menu);
int  tether_ui_menu_get_item_count(TUID menu);
const char* tether_ui_menu_get_item_text(TUID menu, int index);

void tether_ui_menu_open(TUID menu);
void tether_ui_menu_close(TUID menu);
bool tether_ui_menu_is_open(TUID menu);

/* Position anchored relative to a header/trigger entity with top offset */
void tether_ui_menu_position_relative(TUID menu, TUID anchor_header, float offset_y);

/* Position anchored at explicit coordinates (e.g. for context menu) */
void tether_ui_menu_position_at(TUID menu, float x, float y, float width);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_MENU_H */
