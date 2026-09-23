#ifndef TETHER_YAML_H
#define TETHER_YAML_H

#include "tether/core/tether_ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parses a YAML UI file and attaches the resulting UI tree to a specific parent entity.
 * If parent is TETHER_INVALID_GUID, it will attach to the main root.
 * Returns the Root Entity GUID, or TETHER_INVALID_GUID if parsing failed.
 */
Tether_GUID tether_yaml_load(const char* filepath, Tether_GUID parent);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_YAML_H */
