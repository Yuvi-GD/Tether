#ifndef TETHER_ECS_H
#define TETHER_ECS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * TUID: A 64-bit Tether Unique Identifier for an Entity.
 * - Lower 32 bits: The Entity Index (used for fast array lookups).
 * - Upper 32 bits: The Generation version (prevents use-after-free bugs when
 *   indices are recycled).
 */
typedef uint64_t TUID;

#define TETHER_INVALID_TUID 0

/* Extracts the 32-bit index from a TUID */
static inline uint32_t tether_ecs_get_index(TUID tuid) {
  return (uint32_t)(tuid & 0xFFFFFFFF);
}

/* Extracts the 32-bit generation from a TUID */
static inline uint32_t tether_ecs_get_generation(TUID tuid) {
  return (uint32_t)((tuid >> 32) & 0xFFFFFFFF);
}

/* Construct a TUID from index and generation */
static inline TUID tether_ecs_make_tuid(uint32_t index,
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
  TUID *entity_map;        /* Maps Dense Array index BACK to the Entity TUID */
  size_t element_size;     /* Size of one component in bytes */
  uint32_t count;          /* Number of active elements */
  uint32_t capacity;       /* Allocated capacity */
  void *default_data;      /* Default data template (copied on add_component) */
} Tether_DenseArray;

/* ========================================================================== */
/*                       Global Entity Lifecycle                              */
/* ========================================================================== */

/* Initialize the global ECS registry. */
void tether_ecs_init(void);

/* Initialize the global root entities. MUST be called after components are registered. */
void tether_ecs_init_roots(void);

/* Destroy the global ECS registry and free all memory. */
void tether_ecs_term(void);

/* Get the primary root entity for the layout tree. */
TUID tether_ecs_get_main_root(void);

/* Get the global overlay root entity for tooltips and dropdowns. */
TUID tether_ecs_get_overlay_root(void);

/* Exposes the dense array for raw linear iteration (maximum cache locality) */
Tether_DenseArray *tether_ecs_get_dense_array(int component_id);

/* ========================================================================== */
/*                              Entity Lifecycle                              */
/* ========================================================================== */

/* Create a new entity and return its TUID. */
TUID tether_ecs_create_entity(void);

/* Check if an entity is still alive. */
bool tether_ecs_is_valid(TUID entity);

/* Destroy an entity, immediately cascading the destroy to all its children. */
void tether_ecs_destroy_entity(TUID entity);

/*
 * Backwards-compatible string ID lookup.
 * This resolves through the registry layer and is kept for compatibility
 * while the user-facing ID system migrates to registry-backed lookups.
 */
TUID tether_ecs_find_by_id(const char* id);

/* ========================================================================== */
/*                          Component Management                              */
/* ========================================================================== */



/* Register a component array for a specific compile-time component type ID with an optional default byte template (or NULL for zeroed memory). */
void tether_ecs_register_component_static(uint32_t component_id, size_t element_size, const void* default_data);

/* Allocate a custom component ID dynamically for a third-party struct with an optional default byte template. Returns the assigned ID. */
uint32_t tether_ecs_register_component_dynamic(size_t element_size, const void* default_data);

/*
 * Add a component to an entity.
 * Returns a pointer to the uninitialized memory block in the dense array.
 */
void *tether_ecs_add_component(TUID entity, int component_id);

/*
 * Get a component for an entity.
 * Returns NULL if the entity does not have this component.
 */
void *tether_ecs_get_component(TUID entity, int component_id);

/* Remove a component from an entity using Swap-and-Pop. */
void tether_ecs_remove_component(TUID entity, int component_id);

/* ========================================================================== */
/*                            Hierarchy API                                   */
/* ========================================================================== */

/*
 * Safely detach an entity from its parent (stitches siblings together).
 */
void tether_ecs_detach_entity(TUID entity);

/*
 * Attach an entity as the last child of a parent.
 */
void tether_ecs_attach_entity(TUID parent, TUID entity);

/*
 * Bring an entity to the front of its parent's local draw order.
 */
void tether_ecs_bring_to_front(TUID entity);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_ECS_H */
