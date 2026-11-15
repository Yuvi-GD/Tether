#ifndef TETHER_LIST_H
#define TETHER_LIST_H

#include "tether/core/tether_ecs.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Tether List (Base Selection Container)                                     */
/* ========================================================================== */

/* Lifecycle */
TUID tether_list_create(TUID parent);
TUID tether_list_add_item(TUID list);
void        tether_list_remove_item(TUID list, int index);
void        tether_list_clear(TUID list);

/* Item Queries (backed by TETHER_COMPONENT_HIERARCHY) */
TUID tether_list_get_item(TUID list, int index);
int         tether_list_get_count(TUID list);

/* Selection Management (backed by TETHER_COMPONENT_SELECTION_STATE) */
void        tether_list_set_selected(TUID list, int index);
int         tether_list_get_selected(TUID list);
void        tether_list_unselect(TUID list);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_LIST_H */
