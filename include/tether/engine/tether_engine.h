#ifndef TETHER_ENGINE_H
#define TETHER_ENGINE_H

#include <stdbool.h>
#include "tether/tether.h"
#include "tether/core/tether_ecs.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Engine Scene & Realization API (src/engine/tether_engine.c) */

/* Initializes the Render Engine (Creates Main Scene & Registers Roots) */
void tether_engine_init(void);

/* Cleans up the Render Engine (Destroys Main Scene & Unregisters Roots) */
void tether_engine_term(void);

/* Allocates RHI memory for an entity (and children) */
void tether_engine_realize(Tether_GUID entity);

/* Frees RHI memory for an entity (and children) recursively */
void tether_engine_unrealize(Tether_GUID entity);

/*
 * The main Render Engine synchronization function.
 * This function iterates over the ECS Dirty Arrays and pushes updates
 * to the RHI layer using the granular property setters.
 * Returns true if a draw was actually issued (GPU work was submitted).
 */
bool tether_engine_frame(float screen_w, float screen_h, bool force_redraw);


#ifdef __cplusplus
}
#endif

#endif /* TETHER_ENGINE_H */
