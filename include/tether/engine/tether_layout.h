#ifndef TETHER_LAYOUT_H
#define TETHER_LAYOUT_H

#include "tether/core/tether_ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculates the exact pixel bounding boxes (SlotTransform) for the entire ECS hierarchy.
 * @param root The root entity ID.
 * @param screen_width The width of the viewport.
 * @param screen_height The height of the viewport.
 */
void tether_layout_process_tree(Tether_GUID root, float screen_width, float screen_height);

/**
 * Convenience function to process all root entities.
 */
void tether_layout_process_all(float screen_width, float screen_height);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_LAYOUT_H */
