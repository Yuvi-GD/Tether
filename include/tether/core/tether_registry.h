#ifndef TETHER_REGISTRY_H
#define TETHER_REGISTRY_H

#include <stdbool.h>
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
 * Widget factory registration.
 */
void tether_register_widget(const char* name, Tether_WidgetCreateFn create_fn);
Tether_GUID tether_create_widget(const char* name, Tether_GUID parent);

/*
 * String-ID registry: a stable lookup layer for user-facing names.
 * This keeps entity identity in ECS (GUID) while making names queryable.
 */
void tether_registry_alias_entity(Tether_GUID entity, const char* alias);
const char* tether_registry_get_alias(Tether_GUID entity);
Tether_GUID tether_registry_find_entity(const char* alias);
void tether_registry_remove_alias(Tether_GUID entity);

/*
 * Font registry lives here now instead of in the ECS core.
 * These are the public entrypoints used by the renderer and font loader.
 * Max 32 fonts can be registered.
 * ID 0 is reserved for system default font.
 */
uint32_t tether_font_register(const char* name, const char* path);
const char* tether_font_get_path(uint32_t font_id);
const char* tether_font_get_name(uint32_t font_id);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_REGISTRY_H */
