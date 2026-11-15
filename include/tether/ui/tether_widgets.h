#ifndef TETHER_WIDGETS_H
#define TETHER_WIDGETS_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Low-Level Widget Entity Allocation (transforms, layout, slot, visibility, overflow) */
TUID tether_widget_create_container(TUID parent);

/* Hierarchy Management */
void tether_widget_add_to_parent(TUID entity, TUID parent);
void tether_widget_remove_from_parent(TUID entity);

/* Hierarchy Queries (index: 0 = first child, -1 = last child, >0 = nth sibling) */
TUID tether_widget_get_parent(TUID entity);
TUID tether_widget_get_child_at(TUID entity, int index);
int tether_widget_get_child_count(TUID entity);

/* Visibility Management */
void tether_widget_set_visibility(TUID entity, Tether_VisibilityState state);
bool tether_widget_is_visible(TUID entity);
bool tether_widget_is_collapsed(TUID entity);

/* Interaction & State Management */
void tether_widget_add_interaction(TUID entity, Tether_HitBehavior behavior);
void tether_widget_set_enabled(TUID entity, bool enabled);
bool tether_widget_is_enabled(TUID entity);

/* Unified Widget Lifecycle: visibility + RHI realize/unrealize */
void tether_widget_show(TUID entity);
void tether_widget_hide(TUID entity);

/* Cleans up GPU/RHI memory for this widget (and its children) and removes it from ECS */
void tether_widget_destroy(TUID entity);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_WIDGETS_H */
