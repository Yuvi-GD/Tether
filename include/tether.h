#ifndef TETHER_H
#define TETHER_H

typedef struct {
    int width;
    int height;
    const char* title;
} Tether_App_Config;

/* The core engine promise */
void tether_run(Tether_App_Config* config);

#endif