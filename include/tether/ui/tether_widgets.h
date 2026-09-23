#ifndef TETHER_WIDGETS_H
#define TETHER_WIDGETS_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Native Widget Factories */
Tether_GUID tether_widget_create_panel(Tether_GUID parent);
Tether_GUID tether_widget_create_text(Tether_GUID parent);
Tether_GUID tether_widget_create_button(Tether_GUID parent);

/* Hierarchy Management */
void tether_widget_add_to_parent(Tether_GUID entity, Tether_GUID parent);

/* Visibility Management */
void tether_widget_set_visibility(Tether_GUID entity, Tether_VisibilityState state);
bool tether_widget_is_visible(Tether_GUID entity);
bool tether_widget_is_collapsed(Tether_GUID entity);

/* Allocates GPU/RHI memory for this widget (and its children) to actually display it */
void tether_widget_show(Tether_GUID entity);

/* Cleans up GPU/RHI memory for this widget (and its children) and removes it from ECS */
void tether_widget_destroy(Tether_GUID entity);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_WIDGETS_H */
