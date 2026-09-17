#ifndef SANDBOX_EVENTS_H
#define SANDBOX_EVENTS_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"

void on_open_c_modal(Tether_GUID entity, Tether_EventType type, void* user_data);
void on_close_c_modal(Tether_GUID entity, Tether_EventType type, void* user_data);
void on_open_yaml_modal(Tether_GUID entity, Tether_EventType type, void* user_data);
void on_close_yaml_modal(Tether_GUID entity, Tether_EventType type, void* user_data);

void sandbox_bind_events(void);

#endif
