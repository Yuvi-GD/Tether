#ifndef TETHER_REGISTRY_H
#define TETHER_REGISTRY_H

#include "tether/core/tether_ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Signature for a widget creation function.
 * Given a parent GUID, it should create a new entity, add necessary components,
 * attach it to the parent in the hierarchy, and return the new entity GUID.
 */
typedef Tether_GUID (*Tether_WidgetCreateFn)(Tether_GUID parent);

/* Initialize and Terminate the Registry */
void tether_registry_init(void);
void tether_registry_term(void);

/* 
 * Register a widget creation function by name.
 * Example: tether_register_widget("Panel", create_panel_fn);
 */
void tether_register_widget(const char* name, Tether_WidgetCreateFn create_fn);

/* 
 * Instantiates a widget by looking up its name in the registry.
 * Returns TETHER_INVALID_GUID if the name is not registered.
 */
Tether_GUID tether_create_widget(const char* name, Tether_GUID parent);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_REGISTRY_H */
