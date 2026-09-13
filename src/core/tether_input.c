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
        Tether_DenseArray* transforms = tether_ecs_get_dense_array(TETHER_COMPONENT_SLOT_TRANSFORM);
        if (!transforms) return;

        /* Iterate backwards so we hit the top-most rendered element first */
        for (int i = (int)transforms->count - 1; i >= 0; i--) {
            Tether_SlotTransform* t = (Tether_SlotTransform*)((uint8_t*)transforms->data + (i * transforms->element_size));
            
            /* AABB Hit Test */
            if (event->x >= t->x && event->x <= t->x + t->width &&
                event->y >= t->y && event->y <= t->y + t->height) {
                
                Tether_GUID hit_entity = transforms->entity_map[i];

                if (event->button == TETHER_MOUSE_BUTTON_LEFT) {
                    printf("[Sandbox] Left clicked entity %llu at (%.1f, %.1f). Randomizing color!\n", (unsigned long long)hit_entity, event->x, event->y);
                    Tether_Color* c = (Tether_Color*)tether_ecs_get_component(hit_entity, TETHER_COMPONENT_COLOR);
                    if (c) {
                        c->r = (uint8_t)(rand() % 255);
                        c->g = (uint8_t)(rand() % 255);
                        c->b = (uint8_t)(rand() % 255);
                    }
                } 
                else if (event->button == TETHER_MOUSE_BUTTON_RIGHT) {
                    printf("[Sandbox] Right clicked entity %llu at (%.1f, %.1f). Deleting!\n", (unsigned long long)hit_entity, event->x, event->y);
                    tether_ecs_destroy_entity(hit_entity);
                }
                
                /* Only process the top-most clicked entity */
                break;
            }
        }
    }
}
