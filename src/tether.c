#include "tether.h"
#include "backends/tether_hal.h"

void tether_run(Tether_App_Config* config) {
    /* Initialize the Tether UI Kernel (Memory Arena, Event Dispatcher, etc.) */
    /* ... future code ... */

    /* Hand control over to the Hardware Abstraction Layer to start the OS window loop */
    tether_hal_run(config);
}
