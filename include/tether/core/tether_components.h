#ifndef TETHER_COMPONENTS_H
#define TETHER_COMPONENTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Component ID Definitions
 */
#define TETHER_COMPONENT_TRANSFORM 0
#define TETHER_COMPONENT_COLOR     1
#define TETHER_COMPONENT_HIERARCHY 2
#define TETHER_COMPONENT_MAX       3

/*
 * Standard UI Components
 */

typedef struct Tether_Transform {
    float x;
    float y;
    float width;
    float height;
} Tether_Transform;

typedef struct Tether_Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Tether_Color;

typedef struct Tether_Hierarchy {
    uint64_t parent;
    uint64_t first_child;
    uint64_t next_sibling;
    uint32_t child_count;
} Tether_Hierarchy;

#ifdef __cplusplus
}
#endif

#endif /* TETHER_COMPONENTS_H */
