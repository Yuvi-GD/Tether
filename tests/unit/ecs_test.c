#include "tether/core/tether_ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define COMPONENT_TRANSFORM 0

typedef struct {
    float x, y, width, height;
} Transform;

int main() {
    printf("--- Tether UI: ECS Kernel Test ---\n");
    tether_ecs_init();
    
    tether_ecs_register_component_type(COMPONENT_TRANSFORM, sizeof(Transform));
    
    printf("[1] Spawning 10,000 entities with Transform components...\n");
    Tether_GUID entities[10000];
    for (int i = 0; i < 10000; i++) {
        entities[i] = tether_ecs_create_entity();
        Transform* t = (Transform*)tether_ecs_add_component(entities[i], COMPONENT_TRANSFORM);
        t->x = (float)i;
        t->y = (float)i;
    }
    
    Tether_DenseArray* dense = tether_ecs_get_dense_array(COMPONENT_TRANSFORM);
    printf("    -> Dense array count: %u\n", dense->count);
    
    if (dense->count != 10000) {
        printf("    [FAILED] Count should be 10000\n");
        return 1;
    }
    
    printf("[2] Deleting 5,000 entities (every even index)...\n");
    for (int i = 0; i < 10000; i += 2) {
        tether_ecs_destroy_entity(entities[i]);
    }
    
    printf("    -> Dense array count: %u\n", dense->count);
    
    if (dense->count != 5000) {
        printf("    [FAILED] Count should be 5000\n");
        return 1;
    }
    
    printf("[3] Verifying Dense Array is 100%% contiguous with no holes...\n");
    bool valid = true;
    for (uint32_t i = 0; i < dense->count; i++) {
        Tether_GUID e = dense->entity_map[i];
        if (!tether_ecs_is_valid(e)) {
            printf("    [FAILED] Found invalid or dead entity at dense index %u\n", i);
            valid = false;
            break;
        }
        
        // Also check if getting the component back points to the exact dense location
        Transform* t = (Transform*)tether_ecs_get_component(e, COMPONENT_TRANSFORM);
        void* expected_ptr = (uint8_t*)dense->data + (i * dense->element_size);
        if ((void*)t != expected_ptr) {
            printf("    [FAILED] Sparse map pointer mismatch at dense index %u\n", i);
            valid = false;
            break;
        }
    }
    
    if (valid) {
        printf("    [SUCCESS] Memory array is 100%% packed with zero fragmentation.\n");
    }
    
    tether_ecs_term();
    printf("--- Test Complete ---\n");
    
    return valid ? 0 : 1;
}
