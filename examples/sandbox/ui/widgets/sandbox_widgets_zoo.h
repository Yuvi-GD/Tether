#ifndef SANDBOX_WIDGETS_ZOO_H
#define SANDBOX_WIDGETS_ZOO_H

#include "tether/core/tether_ecs.h"

void sandbox_widgets_zoo_cleanup(void);
TUID sandbox_widgets_zoo_create(TUID parent);
bool sandbox_widgets_zoo_check(void);

#endif /* SANDBOX_WIDGETS_ZOO_H */
