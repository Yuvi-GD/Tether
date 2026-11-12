#ifndef TETHER_WIDGETS_H
#define TETHER_WIDGETS_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Low-Level Widget Entity Allocation (transforms, layout, slot, visibility, overflow) */
Tether_GUID tether_widget_create_container(Tether_GUID parent);

/* Hierarchy Management */
void tether_widget_add_to_parent(Tether_GUID entity, Tether_GUID parent);
void tether_widget_remove_from_parent(Tether_GUID entity);

/* Hierarchy Queries (index: 0 = first child, -1 = last child, >0 = nth sibling) */
Tether_GUID tether_widget_get_parent(Tether_GUID entity);
Tether_GUID tether_widget_get_child_at(Tether_GUID entity, int index);
int tether_widget_get_child_count(Tether_GUID entity);

/* Visibility Management */
void tether_widget_set_visibility(Tether_GUID entity, Tether_VisibilityState state);
bool tether_widget_is_visible(Tether_GUID entity);
bool tether_widget_is_collapsed(Tether_GUID entity);

/* Interaction & State Management */
void tether_widget_add_interaction(Tether_GUID entity, Tether_HitBehavior behavior);
void tether_widget_set_enabled(Tether_GUID entity, bool enabled);
bool tether_widget_is_enabled(Tether_GUID entity);

/* Unified Widget Lifecycle: visibility + RHI realize/unrealize */
void tether_widget_show(Tether_GUID entity);
void tether_widget_hide(Tether_GUID entity);

/* Cleans up GPU/RHI memory for this widget (and its children) and removes it from ECS */
void tether_widget_destroy(Tether_GUID entity);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_WIDGETS_H */
