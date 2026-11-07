#ifndef TETHER_H
#define TETHER_H

#include <stdint.h>
#include <stdbool.h>
#include "tether/core/tether_input.h"

/* Tether Version Information (Developer Preview) */
#define TETHER_VERSION_MAJOR 0
#define TETHER_VERSION_MINOR 0
#define TETHER_VERSION_PATCH 2
#define TETHER_VERSION_STRING "0.0.2-preview"

typedef struct Tether_Config {
    int width;
    int height;
    const char* title;
    const char* initial_yaml;
    int target_fps; /* 0 means sync to monitor refresh rate (VSync) */
    
    /* Lifecycle Callbacks */
    void (*on_init)(void);
    void (*on_frame)(void);
    void (*on_post_frame)(void);
    void (*on_term)(void);
} Tether_Config;

/* The primary engine entry point */
void  tether_init(Tether_Config* config);
void  tether_run(Tether_Config* config);
void  tether_queue_redraw(void);
bool  tether_did_draw(void);

/* Lifecycle hooks invoked by platform backend runners (e.g. Chitra) */
void  tether_on_init(void);
void* tether_on_frame(int width, int height);
void  tether_on_post_frame(void);
void  tether_on_cleanup(void);
void  tether_on_input(const Tether_Pointer_Event* event);

#endif /* TETHER_H */
