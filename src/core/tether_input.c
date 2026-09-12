#include "tether/core/tether_input.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include <stdlib.h>
#include <stdio.h>

void tether_input_process_event(Tether_Pointer_Event* event) {
    if (!event) return;

    /* 
     * [SANDBOX LOGIC] 
     * This temporary code tests the UI-ECS and Rasterizer bridges.
     * In Outcome 5, this will be replaced with real spatial hit-testing 
     * against the Transform array to detect hovered/clicked UI elements.
     */
    if (event->type == TETHER_POINTER_DOWN) {
        if (event->button == TETHER_MOUSE_BUTTON_LEFT) {
            printf("[Sandbox] Left click at (%.1f, %.1f) - Spawning 50 entities!\n", event->x, event->y);
            
            for (int i = 0; i < 50; i++) {
                Tether_GUID entity = tether_ecs_create_entity();
                
                Tether_Transform* t = (Tether_Transform*)tether_ecs_add_component(entity, TETHER_COMPONENT_TRANSFORM);
                t->width = (float)(20 + (rand() % 80));
                t->height = (float)(20 + (rand() % 80));
                t->x = (float)(rand() % 800); // Random screen X
                t->y = (float)(rand() % 600); // Random screen Y
                
                Tether_Color* c = (Tether_Color*)tether_ecs_add_component(entity, TETHER_COMPONENT_COLOR);
                c->r = (uint8_t)(rand() % 255);
                c->g = (uint8_t)(rand() % 255);
                c->b = (uint8_t)(rand() % 255);
                c->a = 255;
            }
        } 
        else if (event->button == TETHER_MOUSE_BUTTON_RIGHT) {
            printf("[Sandbox] Right click at (%.1f, %.1f) - Deleting up to 50 entities!\n", event->x, event->y);
            
            Tether_DenseArray* dense = tether_ecs_get_dense_array(TETHER_COMPONENT_TRANSFORM);
            int delete_count = 0;
            
            /* Delete backwards to avoid swap-and-pop issues during iteration */
            while (dense->count > 0 && delete_count < 50) {
                Tether_GUID last_entity = dense->entity_map[dense->count - 1];
                tether_ecs_destroy_entity(last_entity);
                delete_count++;
            }
            printf("[Sandbox] Deleted %d entities. Remaining: %u\n", delete_count, dense->count);
        }
    }
}
