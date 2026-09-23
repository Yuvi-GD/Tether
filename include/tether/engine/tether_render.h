#ifndef TETHER_RENDER_H
#define TETHER_RENDER_H

#include <stdbool.h>
#include "tether/core/tether_ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes the Render Engine (Creates Main Scene & Registers Roots) */
void tether_render_init(void);

/* Allocates RHI memory for an entity (and children) */
void tether_render_realize(Tether_GUID entity);

/* Frees RHI memory for an entity (and children) recursively */
void tether_render_unrealize(Tether_GUID entity);

/*
 * The main Render Engine synchronization function.
 * This function iterates over the ECS Dirty Arrays and pushes updates
 * to the RHI layer using the granular property setters.
 */
void tether_render_frame(float screen_w, float screen_h, bool force_redraw);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_RENDER_H */
