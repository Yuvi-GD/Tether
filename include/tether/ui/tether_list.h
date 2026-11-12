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
Tether_GUID tether_list_create(Tether_GUID parent);
Tether_GUID tether_list_add_item(Tether_GUID list);
void        tether_list_remove_item(Tether_GUID list, int index);
void        tether_list_clear(Tether_GUID list);

/* Item Queries (backed by TETHER_COMPONENT_HIERARCHY) */
Tether_GUID tether_list_get_item(Tether_GUID list, int index);
int         tether_list_get_count(Tether_GUID list);

/* Selection Management (backed by TETHER_COMPONENT_SELECTION_STATE) */
void        tether_list_set_selected(Tether_GUID list, int index);
int         tether_list_get_selected(Tether_GUID list);
void        tether_list_unselect(Tether_GUID list);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_LIST_H */
