#ifndef TETHER_WIDGETS_H
#define TETHER_WIDGETS_H

#include "tether/core/tether_ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Native Widget Factories */
Tether_GUID tether_widget_create_panel(Tether_GUID parent);
Tether_GUID tether_widget_create_text(Tether_GUID parent);
Tether_GUID tether_widget_create_button(Tether_GUID parent);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_WIDGETS_H */
