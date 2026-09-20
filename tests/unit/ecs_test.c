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
    tether_ecs_init_roots();
    
    tether_ecs_register_component_static(COMPONENT_TRANSFORM, sizeof(Transform));
    
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
    int custom_id = tether_ecs_register_component_dynamic(sizeof(CustomStats));
    if (custom_id < TETHER_COMPONENT_MAX) {
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
    tether_ecs_register_component_static(TETHER_COMPONENT_HIERARCHY, sizeof(Tether_Hierarchy)); // 3 is TETHER_COMPONENT_HIERARCHY
    
    Tether_GUID parent = tether_ecs_create_entity();
    Tether_GUID c1 = tether_ecs_create_entity();
    Tether_GUID c2 = tether_ecs_create_entity();
    Tether_GUID c3 = tether_ecs_create_entity();
    
    // Add hierarchy components
    Tether_Hierarchy* hp = (Tether_Hierarchy*)tether_ecs_add_component(parent, TETHER_COMPONENT_HIERARCHY);
    hp->parent = 0; hp->first_child = 0; hp->last_child = 0; hp->prev_sibling = 0; hp->next_sibling = 0; hp->child_count = 0;
    
    Tether_Hierarchy* hc1 = (Tether_Hierarchy*)tether_ecs_add_component(c1, TETHER_COMPONENT_HIERARCHY);
    hc1->parent = 0; hc1->first_child = 0; hc1->last_child = 0; hc1->prev_sibling = 0; hc1->next_sibling = 0; hc1->child_count = 0;
    
    Tether_Hierarchy* hc2 = (Tether_Hierarchy*)tether_ecs_add_component(c2, TETHER_COMPONENT_HIERARCHY);
    hc2->parent = 0; hc2->first_child = 0; hc2->last_child = 0; hc2->prev_sibling = 0; hc2->next_sibling = 0; hc2->child_count = 0;
    
    Tether_Hierarchy* hc3 = (Tether_Hierarchy*)tether_ecs_add_component(c3, TETHER_COMPONENT_HIERARCHY);
    hc3->parent = 0; hc3->first_child = 0; hc3->last_child = 0; hc3->prev_sibling = 0; hc3->next_sibling = 0; hc3->child_count = 0;
    
    // Attach c1, c2, c3 to parent
    tether_ecs_attach_entity(parent, c1);
    tether_ecs_attach_entity(parent, c2);
    tether_ecs_attach_entity(parent, c3);
    
    Tether_Hierarchy* ph = (Tether_Hierarchy*)tether_ecs_get_component(parent, TETHER_COMPONENT_HIERARCHY);
    if (ph->first_child != c1 || ph->last_child != c3 || ph->child_count != 3) {
        printf("    [FAILED] Parent hierarchy not setup correctly\n");
        valid = false;
    }
    
    // Detach c2 (middle child)
    tether_ecs_detach_entity(c2);
    
    ph = (Tether_Hierarchy*)tether_ecs_get_component(parent, TETHER_COMPONENT_HIERARCHY);
    if (ph->child_count != 2) {
        printf("    [FAILED] Detach failed to update child count\n");
        valid = false;
    }
    
    Tether_Hierarchy* h1 = (Tether_Hierarchy*)tether_ecs_get_component(c1, TETHER_COMPONENT_HIERARCHY);
    Tether_Hierarchy* h3 = (Tether_Hierarchy*)tether_ecs_get_component(c3, TETHER_COMPONENT_HIERARCHY);
    if (h1->next_sibling != c3 || h3->prev_sibling != c1) {
        printf("    [FAILED] O(1) Doubly-linked list stitch failed\n");
        valid = false;
    } else {
        printf("    [SUCCESS] Hierarchy attachment and detachment works.\n");
    }
    
    printf("[6] Testing Local Z-Index Bring To Front...\n");
    tether_ecs_bring_to_front(c1); // C1 was first, now it should be last!
    ph = (Tether_Hierarchy*)tether_ecs_get_component(parent, TETHER_COMPONENT_HIERARCHY);
    if (ph->last_child != c1 || ph->first_child != c3) {
        printf("    [FAILED] Bring to front did not update parent pointers properly\n");
        valid = false;
    } else {
        printf("    [SUCCESS] Bring to front instantly swapped C1 to last_child!\n");
    }
    
    // Test Overlay Root
    printf("[7] Testing Global Overlay Root...\n");
    Tether_GUID overlay = tether_ecs_get_overlay_root();
    Tether_GUID main_r = tether_ecs_get_main_root();
    if (!tether_ecs_is_valid(overlay) || !tether_ecs_is_valid(main_r) || overlay == main_r) {
        printf("    [FAILED] Global overlay root not initialized properly\n");
        valid = false;
    } else {
        printf("    [SUCCESS] Main Root and Overlay Root are correctly isolated.\n");
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
