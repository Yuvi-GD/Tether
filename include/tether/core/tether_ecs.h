#ifndef TETHER_ECS_H
#define TETHER_ECS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Tether_GUID: A 64-bit Globally Unique Identifier for an Entity.
 * - Lower 32 bits: The Entity Index (used for fast array lookups).
 * - Upper 32 bits: The Generation version (prevents use-after-free bugs when
 * indices are recycled).
 */
typedef uint64_t Tether_GUID;

#define TETHER_INVALID_GUID 0

/* Extracts the 32-bit index from a GUID */
static inline uint32_t tether_ecs_get_index(Tether_GUID guid) {
  return (uint32_t)(guid & 0xFFFFFFFF);
}

/* Extracts the 32-bit generation from a GUID */
static inline uint32_t tether_ecs_get_generation(Tether_GUID guid) {
  return (uint32_t)((guid >> 32) & 0xFFFFFFFF);
}

/* Construct a GUID from index and generation */
static inline Tether_GUID tether_ecs_make_guid(uint32_t index,
                                               uint32_t generation) {
  return ((uint64_t)generation << 32) | index;
}

/*
 * Sparse Map: Maps an Entity Index to its position in a Dense Array.
 * The sparse array index is the Entity Index.
 * The value stored is the index in the Dense Array.
 */
#define TETHER_SPARSE_INVALID_INDEX 0xFFFFFFFF

typedef struct Tether_SparseMap {
  uint32_t *dense_indices;
  uint32_t capacity;
} Tether_SparseMap;

/*
 * Dense Array: A perfectly contiguous block of component data.
 */
typedef struct Tether_DenseArray {
  void *data;              /* Raw block of memory */
  Tether_GUID *entity_map; /* Maps Dense Array index BACK to the Entity GUID */
  size_t element_size;     /* Size of one component in bytes */
  uint32_t count;          /* Number of active elements */
  uint32_t capacity;       /* Allocated capacity */
} Tether_DenseArray;

// ============================================================
/* --- Global ECS Lifecycle --- */
// ============================================================

/* Initialize the global ECS registry. */
void tether_ecs_init(void);

/* Initialize the global root entities. MUST be called after components are registered. */
void tether_ecs_init_roots(void);

/* Destroy the global ECS registry and free all memory. */
void tether_ecs_term(void);

/* Get the primary root entity for the layout tree. */
Tether_GUID tether_ecs_get_main_root(void);

/* Get the global overlay root entity for tooltips and dropdowns. */
Tether_GUID tether_ecs_get_overlay_root(void);

// ============================================================
/* --- Entity Lifecycle --- */
// ============================================================

/* Create a new entity and return its GUID. */
Tether_GUID tether_ecs_create_entity(void);

/* Destroy an entity, immediately cascading the destroy to all its children. */
void tether_ecs_destroy_entity(Tether_GUID entity);

/*
 * Backwards-compatible string ID lookup.
 * This resolves through the registry layer and is kept for compatibility
 * while the user-facing ID system migrates to registry-backed lookups.
 */
Tether_GUID tether_ecs_find_by_id(const char* id);

/* Check if an entity is still alive. */
bool tether_ecs_is_valid(Tether_GUID entity);

// ============================================================
/* --- Component Management --- */
// ============================================================

/* Register a component array for a specific compile-time component type ID. */
void tether_ecs_register_component_static(uint32_t component_id, size_t element_size);

/* Allocate a custom component ID dynamically for a third-party struct. Returns the assigned ID. */
uint32_t tether_ecs_register_component_dynamic(size_t element_size);

/*
 * Set the exclusive limit of the statically assigned component IDs.
 * This determines the first ID available to runtime-allocated components.
*/
void tether_ecs_set_static_component_limit(uint32_t component_limit);

/*
 * Add a component to an entity.
 * Returns a pointer to the uninitialized memory block in the dense array.
 */
void *tether_ecs_add_component(Tether_GUID entity, int component_id);

/*
 * Get a component for an entity.
 * Returns NULL if the entity does not have this component.
 */
void *tether_ecs_get_component(Tether_GUID entity, int component_id);

/* Remove a component from an entity using Swap-and-Pop. */
void tether_ecs_remove_component(Tether_GUID entity, int component_id);

// ============================================================
/* --- Hierarchy API --- */
// ============================================================

/*
 * Safely detach an entity from its parent (stitches siblings together).
 */
void tether_ecs_detach_entity(Tether_GUID entity);

/*
 * Attach an entity as the last child of a parent.
 */
void tether_ecs_attach_entity(Tether_GUID parent, Tether_GUID entity);

/*
 * Bring an entity to the front of its parent's local draw order.
 */
void tether_ecs_bring_to_front(Tether_GUID entity);

// ============================================================
/* --- Internal API (Exposed for testing/advanced usage) --- */
// ============================================================

/* Exposes the dense array for raw linear iteration (maximum cache locality) */
Tether_DenseArray *tether_ecs_get_dense_array(int component_id);

/* --- Text Component API --- */

/* Retrieves the character pointer from whichever Text Component the entity has attached. Returns NULL if none. */
const char* tether_ecs_get_text_string(Tether_GUID entity);

/* Stores text in the smallest component that can hold it. */
bool tether_ecs_set_text_string(Tether_GUID entity, const char* string);


#ifdef __cplusplus
}
#endif

#endif /* TETHER_ECS_H */
