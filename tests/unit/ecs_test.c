#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
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
    
    printf("[4] Testing Custom Component Registration...\n");
    typedef struct { int hp; int mp; } CustomStats;
    int custom_id = tether_ecs_allocate_custom_component(sizeof(CustomStats));
    if (custom_id < 32) {
        printf("    [FAILED] Custom component ID should be >= 32 (got %d)\n", custom_id);
        valid = false;
    } else {
        Tether_GUID p1 = tether_ecs_create_entity();
        CustomStats* stats = (CustomStats*)tether_ecs_add_component(p1, custom_id);
        stats->hp = 100;
        stats->mp = 50;
        CustomStats* check = (CustomStats*)tether_ecs_get_component(p1, custom_id);
        if (check->hp != 100 || check->mp != 50) {
            printf("    [FAILED] Custom component data mismatch\n");
            valid = false;
        }
        printf("    [SUCCESS] Custom Component Registration works (ID: %d)\n", custom_id);
    }
    
    printf("[5] Testing Hierarchy Doubly-Linked List and Cascading Delete...\n");
    // Ensure TETHER_COMPONENT_HIERARCHY is registered
    tether_ecs_register_component_type(3, sizeof(Tether_Hierarchy)); // 3 is TETHER_COMPONENT_HIERARCHY
    
    Tether_GUID parent = tether_ecs_create_entity();
    Tether_GUID c1 = tether_ecs_create_entity();
    Tether_GUID c2 = tether_ecs_create_entity();
    Tether_GUID c3 = tether_ecs_create_entity();
    
    // Explicitly add components as required by the new strict decoupled design
    Tether_Hierarchy* hp = (Tether_Hierarchy*)tether_ecs_add_component(parent, 3);
    if (hp) memset(hp, 0, sizeof(Tether_Hierarchy));
    
    Tether_Hierarchy* hc1 = (Tether_Hierarchy*)tether_ecs_add_component(c1, 3);
    if (hc1) memset(hc1, 0, sizeof(Tether_Hierarchy));
    
    Tether_Hierarchy* hc2 = (Tether_Hierarchy*)tether_ecs_add_component(c2, 3);
    if (hc2) memset(hc2, 0, sizeof(Tether_Hierarchy));
    
    Tether_Hierarchy* hc3 = (Tether_Hierarchy*)tether_ecs_add_component(c3, 3);
    if (hc3) memset(hc3, 0, sizeof(Tether_Hierarchy));
    
    tether_ecs_attach_entity(parent, c1);
    tether_ecs_attach_entity(parent, c2);
    tether_ecs_attach_entity(parent, c3);
    
    Tether_Hierarchy* ph = (Tether_Hierarchy*)tether_ecs_get_component(parent, 3);
    if (!ph || ph->child_count != 3 || ph->first_child != c1) {
        printf("    [FAILED] Parent hierarchy not setup correctly\n");
        valid = false;
    }
    
    // Detach C2
    tether_ecs_detach_entity(c2);
    
    ph = (Tether_Hierarchy*)tether_ecs_get_component(parent, 3);
    if (!ph || ph->child_count != 2) {
        printf("    [FAILED] Detach failed to update child count\n");
        valid = false;
    }
    
    Tether_Hierarchy* h1 = (Tether_Hierarchy*)tether_ecs_get_component(c1, 3);
    Tether_Hierarchy* h3 = (Tether_Hierarchy*)tether_ecs_get_component(c3, 3);
    if (h1->next_sibling != c3 || h3->prev_sibling != c1) {
        printf("    [FAILED] O(1) Doubly-linked list stitch failed\n");
        valid = false;
    } else {
        printf("    [SUCCESS] Detach seamlessly stitched C1 and C3 together\n");
    }
    
    // Destroy parent (cascading delete)
    tether_ecs_destroy_entity(parent);
    
    if (tether_ecs_is_valid(parent) || tether_ecs_is_valid(c1) || tether_ecs_is_valid(c3)) {
        printf("    [FAILED] Cascading delete failed. Parent or attached children still alive.\n");
        valid = false;
    } else if (!tether_ecs_is_valid(c2)) {
        printf("    [FAILED] Cascading delete incorrectly killed the detached child (c2).\n");
        valid = false;
    } else {
        printf("    [SUCCESS] Cascading delete correctly wiped sub-tree and spared detached children!\n");
    }
    
    tether_ecs_term();
    printf("--- Test Complete ---\n");
    
    return valid ? 0 : 1;
}
