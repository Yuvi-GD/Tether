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
    void (*on_frame)(void);
    uint32_t max_render_width;
    uint32_t max_render_height;
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

/* Returns true if the last frame actually produced GPU work */
bool tether_engine_did_draw(void);

/* Returns the actual offscreen texture dimensions (may be capped) */
void tether_engine_get_texture_size(uint32_t* out_w, uint32_t* out_h);

void tether_engine_term(void);

#ifdef __cplusplus
}
#endif

#endif // TETHER_ENGINE_H