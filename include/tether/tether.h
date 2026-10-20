#ifndef TETHER_H
#define TETHER_H

#include <stdint.h>

typedef struct {
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
} Tether_App_Config;

/* The core engine entry point */
void tether_run(Tether_App_Config* config);

#endif