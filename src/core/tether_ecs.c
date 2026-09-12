#include "tether/core/tether_ecs.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_ENTITY_CAPACITY 1024
#define INITIAL_DENSE_CAPACITY 64

typedef struct {
  uint32_t *generations;

  uint32_t *free_indices;
  uint32_t free_count;
  uint32_t free_capacity;

  uint32_t next_entity_index;
  uint32_t entity_capacity;

  Tether_SparseMap sparse_maps[TETHER_MAX_COMPONENT_TYPES];
  Tether_DenseArray dense_arrays[TETHER_MAX_COMPONENT_TYPES];
} Tether_Registry;

static Tether_Registry g_registry = {0};

void tether_ecs_init(void) {
  g_registry.entity_capacity = INITIAL_ENTITY_CAPACITY;
  g_registry.generations =
      (uint32_t *)calloc(g_registry.entity_capacity, sizeof(uint32_t));

  g_registry.free_capacity = INITIAL_ENTITY_CAPACITY;
  g_registry.free_indices =
      (uint32_t *)malloc(g_registry.free_capacity * sizeof(uint32_t));
  g_registry.free_count = 0;

  /* Index 0 is reserved for TETHER_INVALID_GUID */
  g_registry.next_entity_index = 1;

  for (int i = 0; i < TETHER_MAX_COMPONENT_TYPES; ++i) {
    g_registry.sparse_maps[i].capacity = 0;
    g_registry.sparse_maps[i].dense_indices = NULL;

    g_registry.dense_arrays[i].capacity = 0;
    g_registry.dense_arrays[i].count = 0;
    g_registry.dense_arrays[i].data = NULL;
    g_registry.dense_arrays[i].entity_map = NULL;
    g_registry.dense_arrays[i].element_size = 0;
  }
}

void tether_ecs_term(void) {
  free(g_registry.generations);
  free(g_registry.free_indices);

  for (int i = 0; i < TETHER_MAX_COMPONENT_TYPES; ++i) {
    free(g_registry.sparse_maps[i].dense_indices);
    free(g_registry.dense_arrays[i].data);
    free(g_registry.dense_arrays[i].entity_map);
  }

  memset(&g_registry, 0, sizeof(Tether_Registry));
}

Tether_GUID tether_ecs_create_entity(void) {
  uint32_t index;
  if (g_registry.free_count > 0) {
    index = g_registry.free_indices[--g_registry.free_count];
  } else {
    if (g_registry.next_entity_index >= g_registry.entity_capacity) {
      uint32_t old_cap = g_registry.entity_capacity;
      g_registry.entity_capacity *= 2;
      g_registry.generations =
          (uint32_t *)realloc(g_registry.generations,
                              g_registry.entity_capacity * sizeof(uint32_t));
      memset(g_registry.generations + old_cap, 0,
             (g_registry.entity_capacity - old_cap) * sizeof(uint32_t));
    }
    index = g_registry.next_entity_index++;
  }

  uint32_t generation = g_registry.generations[index];
  return tether_ecs_make_guid(index, generation);
}

bool tether_ecs_is_valid(Tether_GUID entity) {
  if (entity == TETHER_INVALID_GUID)
    return false;
  uint32_t index = tether_ecs_get_index(entity);
  uint32_t generation = tether_ecs_get_generation(entity);
  if (index == 0 || index >= g_registry.next_entity_index)
    return false;
  return g_registry.generations[index] == generation;
}

void tether_ecs_destroy_entity(Tether_GUID entity) {
  if (!tether_ecs_is_valid(entity))
    return;

  uint32_t index = tether_ecs_get_index(entity);

  /* Remove all components for this entity using swap-and-pop */
  for (int i = 0; i < TETHER_MAX_COMPONENT_TYPES; ++i) {
    if (g_registry.dense_arrays[i].element_size > 0) {
      tether_ecs_remove_component(entity, i);
    }
  }

  /* Increment generation to invalidate old GUIDs */
  g_registry.generations[index]++;

  /* Add to free list */
  if (g_registry.free_count >= g_registry.free_capacity) {
    g_registry.free_capacity *= 2;
    g_registry.free_indices = (uint32_t *)realloc(
        g_registry.free_indices, g_registry.free_capacity * sizeof(uint32_t));
  }
  g_registry.free_indices[g_registry.free_count++] = index;
}

void tether_ecs_register_component_type(int component_id, size_t element_size) {
  if (component_id < 0 || component_id >= TETHER_MAX_COMPONENT_TYPES)
    return;
  g_registry.dense_arrays[component_id].element_size = element_size;
}

static void ensure_sparse_capacity(int component_id,
                                   uint32_t required_capacity) {
  Tether_SparseMap *sparse = &g_registry.sparse_maps[component_id];
  if (required_capacity > sparse->capacity) {
    uint32_t old_cap = sparse->capacity;
    sparse->capacity = required_capacity;
    sparse->dense_indices = (uint32_t *)realloc(
        sparse->dense_indices, sparse->capacity * sizeof(uint32_t));
    for (uint32_t i = old_cap; i < sparse->capacity; ++i) {
      sparse->dense_indices[i] = TETHER_SPARSE_INVALID_INDEX;
    }
  }
}

void *tether_ecs_add_component(Tether_GUID entity, int component_id) {
  if (!tether_ecs_is_valid(entity) || component_id < 0 ||
      component_id >= TETHER_MAX_COMPONENT_TYPES)
    return NULL;

  Tether_DenseArray *dense = &g_registry.dense_arrays[component_id];
  if (dense->element_size == 0)
    return NULL;

  uint32_t entity_index = tether_ecs_get_index(entity);
  ensure_sparse_capacity(component_id, entity_index + 1);

  Tether_SparseMap *sparse = &g_registry.sparse_maps[component_id];

  /* If it already has the component, return the existing one */
  if (sparse->dense_indices[entity_index] != TETHER_SPARSE_INVALID_INDEX) {
    uint32_t dense_idx = sparse->dense_indices[entity_index];
    return (uint8_t *)dense->data + (dense_idx * dense->element_size);
  }

  /* Add new component */
  if (dense->count >= dense->capacity) {
    dense->capacity =
        dense->capacity == 0 ? INITIAL_DENSE_CAPACITY : dense->capacity * 2;
    dense->data = realloc(dense->data, dense->capacity * dense->element_size);
    dense->entity_map = (Tether_GUID *)realloc(
        dense->entity_map, dense->capacity * sizeof(Tether_GUID));
  }

  uint32_t dense_idx = dense->count++;
  sparse->dense_indices[entity_index] = dense_idx;
  dense->entity_map[dense_idx] = entity;

  void *ptr = (uint8_t *)dense->data + (dense_idx * dense->element_size);
  memset(ptr, 0, dense->element_size);
  return ptr;
}

void *tether_ecs_get_component(Tether_GUID entity, int component_id) {
  if (!tether_ecs_is_valid(entity) || component_id < 0 ||
      component_id >= TETHER_MAX_COMPONENT_TYPES)
    return NULL;

  uint32_t entity_index = tether_ecs_get_index(entity);
  Tether_SparseMap *sparse = &g_registry.sparse_maps[component_id];
  if (entity_index >= sparse->capacity ||
      sparse->dense_indices[entity_index] == TETHER_SPARSE_INVALID_INDEX) {
    return NULL;
  }

  Tether_DenseArray *dense = &g_registry.dense_arrays[component_id];
  uint32_t dense_idx = sparse->dense_indices[entity_index];
  return (uint8_t *)dense->data + (dense_idx * dense->element_size);
}

void tether_ecs_remove_component(Tether_GUID entity, int component_id) {
  if (!tether_ecs_is_valid(entity) || component_id < 0 ||
      component_id >= TETHER_MAX_COMPONENT_TYPES)
    return;

  uint32_t entity_index = tether_ecs_get_index(entity);
  Tether_SparseMap *sparse = &g_registry.sparse_maps[component_id];

  if (entity_index >= sparse->capacity ||
      sparse->dense_indices[entity_index] == TETHER_SPARSE_INVALID_INDEX) {
    return; /* Doesn't have this component */
  }

  Tether_DenseArray *dense = &g_registry.dense_arrays[component_id];
  uint32_t dense_idx_to_remove = sparse->dense_indices[entity_index];
  uint32_t last_dense_idx = dense->count - 1;

  /* Swap and pop if we are not removing the last element */
  if (dense_idx_to_remove != last_dense_idx) {
    void *dst =
        (uint8_t *)dense->data + (dense_idx_to_remove * dense->element_size);
    void *src = (uint8_t *)dense->data + (last_dense_idx * dense->element_size);
    memcpy(dst, src, dense->element_size);

    /* Update the entity map and the sparse map for the swapped entity */
    Tether_GUID swapped_entity = dense->entity_map[last_dense_idx];
    dense->entity_map[dense_idx_to_remove] = swapped_entity;

    uint32_t swapped_entity_index = tether_ecs_get_index(swapped_entity);
    sparse->dense_indices[swapped_entity_index] = dense_idx_to_remove;
  }

  /* Invalidate the removed entity's sparse map entry */
  sparse->dense_indices[entity_index] = TETHER_SPARSE_INVALID_INDEX;
  dense->count--;
}

Tether_DenseArray *tether_ecs_get_dense_array(int component_id) {
  if (component_id < 0 || component_id >= TETHER_MAX_COMPONENT_TYPES)
    return NULL;
  return &g_registry.dense_arrays[component_id];
}
