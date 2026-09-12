#ifndef TETHER_YAML_H
#define TETHER_YAML_H

#include "tether/core/tether_ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parses a YAML UI file and automatically creates ECS entities
 * and populates their components.
 * Returns the Root Entity GUID, or TETHER_INVALID_GUID if parsing failed.
 */
Tether_GUID tether_yaml_load(const char* filepath);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_YAML_H */
