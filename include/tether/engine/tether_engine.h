#ifndef TETHER_ENGINE_H
#define TETHER_ENGINE_H

#include <stdbool.h>
#include "tether/core/tether_input.h" // For Tether_Pointer_Event

#ifdef __cplusplus
extern "C" {
#endif

/* Engine configuration */
typedef struct {
    void (*on_init)(void);
} Tether_EngineConfig;

void tether_engine_init(Tether_EngineConfig* config);

/* Direct compile-time bindings for HAL to call */
void tether_engine_on_hal_init(int width, int height, const void* device, const void* instance);

/* Returns the opaque raster texture handle for the HAL to blit to the screen */
void* tether_engine_on_hal_frame(int width, int height);

void tether_engine_on_hal_input(const Tether_Pointer_Event* event);

void tether_engine_on_hal_cleanup(void);
void tether_engine_on_hal_post_cleanup(void);

/* Triggers a layout and render pass for the next frame */
void tether_engine_queue_redraw(void);

void tether_engine_term(void);

#ifdef __cplusplus
}
#endif

#endif // TETHER_ENGINE_H