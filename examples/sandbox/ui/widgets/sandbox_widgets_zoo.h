#ifndef SANDBOX_WIDGETS_ZOO_H
#define SANDBOX_WIDGETS_ZOO_H

#include "tether/core/tether_ecs.h"

void sandbox_widgets_zoo_cleanup(void);
Tether_GUID sandbox_widgets_zoo_create(Tether_GUID parent);
bool sandbox_widgets_zoo_check(void);

#endif /* SANDBOX_WIDGETS_ZOO_H */
