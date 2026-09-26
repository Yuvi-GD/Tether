#ifndef TETHER_H
#define TETHER_H

#include <stdint.h>

typedef enum {
    TETHER_MEMORY_HIGH_WATER_MARK = 0, /* Default: Keep allocated memory for maximum speed */
    TETHER_MEMORY_AGGRESSIVE_SHRINK    /* Strict: Shrink memory back to OS when entities are deleted */
} Tether_Memory_Strategy;

typedef enum {
    TETHER_RENDERER_WEBGPU = 0,        /* Default: Hardware Accelerated via ThorVG/WebGPU */
    TETHER_RENDERER_SOFTWARE_CPU       /* Fallback: Pure CPU Rasterization (Future) */
} Tether_Renderer_Backend;

typedef struct {
    int width;
    int height;
    const char* title;
    const char* initial_yaml;
    
    /* Engine Behavior Configuration */
    Tether_Memory_Strategy memory_strategy;
    Tether_Renderer_Backend renderer;
    int target_fps; /* 0 means sync to monitor refresh rate (VSync) */
    
    /* Optional: Cap ThorVG offscreen render resolution to save VRAM on 4K+ displays.
     * 0 means no cap (native resolution). */
    uint32_t max_render_width;
    uint32_t max_render_height;
    
    /* Lifecycle Callbacks */
    void (*on_init)(void);
    void (*on_frame)(void);
} Tether_App_Config;

/* The core engine promise */
void tether_run(Tether_App_Config* config);

#endif