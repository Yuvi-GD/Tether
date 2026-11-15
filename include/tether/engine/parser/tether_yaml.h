#ifndef TETHER_YAML_H
#define TETHER_YAML_H

#include "tether/core/tether_ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parses a YAML UI file and attaches the resulting UI tree to a specific parent entity.
 * If parent is TETHER_INVALID_TUID, it will attach to the main root.
 * Returns the Root Entity TUID, or TETHER_INVALID_TUID if parsing failed.
 */
TUID tether_yaml_load(const char* filepath, TUID parent);

/* Release cached definitions on the UI thread, outside a load. */
void tether_yaml_term(void);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_YAML_H */
