#ifndef SANDBOX_EVENTS_H
#define SANDBOX_EVENTS_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"

void sandbox_bind_events(void);
void sandbox_showcase_update(void);
void sandbox_showcase_close_overlays(void);
void sandbox_showcase_cleanup(void);
bool sandbox_showcase_check(void);

#endif
