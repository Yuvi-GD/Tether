#ifndef TETHER_HAL_H
#define TETHER_HAL_H

#include "tether.h"

/* The Core Engine calls this to start the windowing system.
 * The implementation (e.g., sokol_hal.c) will take over the main thread.
 */
void tether_hal_run(Tether_App_Config* config);

#endif
