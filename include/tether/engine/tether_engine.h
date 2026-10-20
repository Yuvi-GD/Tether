#ifndef TETHER_ENGINE_H
#define TETHER_ENGINE_H

#include <stdbool.h>
#include "tether/tether.h"
#include "tether/core/tether_input.h" // For Tether_Pointer_Event

#ifdef __cplusplus
extern "C" {
#endif

/* Engine configuration */
typedef struct {
    void (*on_init)(void);
    void (*on_frame)(void);
    void (*on_post_frame)(void);
    void (*on_term)(void);
    const char* initial_yaml;
} Tether_EngineConfig;

void tether_engine_init(Tether_EngineConfig* config);
void tether_engine_run(Tether_App_Config* config);

/* Direct backend lifecycle bindings into the Tether Engine */
void  tether_engine_on_init(void);
void* tether_engine_on_frame(int width, int height);
void  tether_engine_on_post_frame(void);
void  tether_engine_on_cleanup(void);
void  tether_engine_on_input(const Tether_Pointer_Event* event);

/* Triggers a layout and render pass for the next frame */
void tether_engine_queue_redraw(void);

/* Returns true if the last frame actually produced GPU work */
bool tether_engine_did_draw(void);

#ifdef __cplusplus
}
#endif

#endif // TETHER_ENGINE_H