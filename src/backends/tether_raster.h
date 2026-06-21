#ifndef TETHER_RASTER_H
#define TETHER_RASTER_H

#include <stdint.h>

/* The HAL calls this when the window is initialized.
 * For WebGPU backends, 'device' and 'instance' are passed as const void* to avoid WebGPU header coupling.
 */
void tether_raster_init(uint32_t width, uint32_t height, const void* device, const void* instance);

/* The HAL calls this when the window resizes */
void tether_raster_resize(uint32_t width, uint32_t height);

/* The HAL calls this every frame to draw the UI */
void tether_raster_draw(void);

/* The HAL calls this to get the rendered texture.
 * For WebGPU backends, this returns the WGPUTexture (as const void*).
 */
const void* tether_raster_get_texture(void);

/* The HAL calls this on teardown */
void tether_raster_term(void);

#endif
