#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"

#include <stdlib.h>
#include <string.h>


#define INITIAL_ENTITY_CAPACITY 1024
#define INITIAL_DENSE_CAPACITY 64
#define INITIAL_COMPONENT_TYPE_CAPACITY (TETHER_COMPONENT_MAX + 32)

typedef struct {
  uint32_t *generations;

  uint32_t *free_indices;
  uint32_t free_count;
  uint32_t free_capacity;

  uint32_t next_entity_index;
  uint32_t entity_capacity;

  uint32_t component_type_capacity;
  Tether_SparseMap *sparse_maps;
  Tether_DenseArray *dense_arrays;
  uint32_t next_custom_component_id;

  struct {
      char names[32][64];
      char paths[32][256];
    uint32_t count;
  } font_registry;
  
  Tether_GUID main_root;
  Tether_GUID overlay_root;
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
  
  g_registry.component_type_capacity = INITIAL_COMPONENT_TYPE_CAPACITY;
  g_registry.sparse_maps = (Tether_SparseMap *)calloc(g_registry.component_type_capacity, sizeof(Tether_SparseMap));
  g_registry.dense_arrays = (Tether_DenseArray *)calloc(g_registry.component_type_capacity, sizeof(Tether_DenseArray));
  
  /* Custom components start exactly where the user ID block ends */
  g_registry.next_custom_component_id = TETHER_COMPONENT_MAX;

  for (uint32_t i = 0; i < g_registry.component_type_capacity; ++i) {
    g_registry.sparse_maps[i].capacity = 0;
    g_registry.sparse_maps[i].dense_indices = NULL;

    g_registry.dense_arrays[i].capacity = 0;
    g_registry.dense_arrays[i].count = 0;
    g_registry.dense_arrays[i].data = NULL;
    g_registry.dense_arrays[i].entity_map = NULL;
    g_registry.dense_arrays[i].element_size = 0;
  }
}

void tether_ecs_init_roots(void) {
  /* Allocate the global roots */
  g_registry.main_root = tether_ecs_create_entity();
  g_registry.overlay_root = tether_ecs_create_entity();
  
  Tether_Hierarchy* hm = (Tether_Hierarchy*)tether_ecs_add_component(g_registry.main_root, TETHER_COMPONENT_HIERARCHY);
  if (hm) memset(hm, 0, sizeof(Tether_Hierarchy));
  tether_ecs_add_component(g_registry.main_root, TETHER_COMPONENT_DIRTY_HIERARCHY);
  tether_ecs_add_component(g_registry.main_root, TETHER_COMPONENT_DIRTY_LAYOUT);
  tether_ecs_add_component(g_registry.main_root, TETHER_COMPONENT_DIRTY_VISUAL);
  
  Tether_Hierarchy* ho = (Tether_Hierarchy*)tether_ecs_add_component(g_registry.overlay_root, TETHER_COMPONENT_HIERARCHY);
  if (ho) memset(ho, 0, sizeof(Tether_Hierarchy));
  tether_ecs_add_component(g_registry.overlay_root, TETHER_COMPONENT_DIRTY_HIERARCHY);
  tether_ecs_add_component(g_registry.overlay_root, TETHER_COMPONENT_DIRTY_LAYOUT);
  tether_ecs_add_component(g_registry.overlay_root, TETHER_COMPONENT_DIRTY_VISUAL);
}

void tether_ecs_term(void) {
  free(g_registry.generations);
  free(g_registry.free_indices);

  if (g_registry.sparse_maps) {
      for (uint32_t i = 0; i < g_registry.component_type_capacity; ++i) {
        if (g_registry.sparse_maps[i].dense_indices) {
          free(g_registry.sparse_maps[i].dense_indices);
        }
        if (g_registry.dense_arrays[i].data) {
          free(g_registry.dense_arrays[i].data);
        }
        if (g_registry.dense_arrays[i].entity_map) {
          free(g_registry.dense_arrays[i].entity_map);
        }
      }
      free(g_registry.sparse_maps);
      free(g_registry.dense_arrays);
      g_registry.sparse_maps = NULL;
      g_registry.dense_arrays = NULL;
  }

  memset(&g_registry, 0, sizeof(Tether_Registry));
}

Tether_GUID tether_ecs_get_main_root(void) {
  return g_registry.main_root;
}

Tether_GUID tether_ecs_get_overlay_root(void) {
  return g_registry.overlay_root;
}

void tether_ecs_set_static_component_limit(uint32_t component_limit) {
  if (component_limit > g_registry.next_custom_component_id) {
    g_registry.next_custom_component_id = component_limit;
  }
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

  Tether_Hierarchy* h_ptr = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
  if (h_ptr) {
    Tether_Hierarchy h_copy = *h_ptr;
    
    // Always detach from parent if we have one
    if (h_copy.parent != 0) {
      tether_ecs_detach_entity(entity);
    }
    
    // Recursively destroy all children (cascading delete)
    Tether_GUID child = h_copy.first_child;
    while (child != 0) { // TETHER_INVALID_GUID is 0
      Tether_Hierarchy* child_h = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
      Tether_GUID next = child_h ? child_h->next_sibling : 0;
      tether_ecs_destroy_entity(child);
      child = next;
    }
  }

  uint32_t index = tether_ecs_get_index(entity);

  /* If this entity has a Dynamic Text component, free its heap allocation! */
  Tether_TextDynamic* dyn = (Tether_TextDynamic*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_DYNAMIC);
  if (dyn && dyn->data) {
      free(dyn->data);
      dyn->data = NULL;
  }

  /* Remove all components for this entity using swap-and-pop */
  for (uint32_t i = 0; i < g_registry.component_type_capacity; ++i) {
      tether_ecs_remove_component(entity, i);
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

void tether_ecs_register_component_static(uint32_t component_id, size_t element_size) {
  if (component_id < 0) return;
  
  if ((uint32_t)component_id >= g_registry.component_type_capacity) {
      uint32_t old_cap = g_registry.component_type_capacity;
      uint32_t new_cap = old_cap;
      while ((uint32_t)component_id >= new_cap) {
          new_cap *= 2;
      }
      g_registry.sparse_maps = (Tether_SparseMap *)realloc(g_registry.sparse_maps, new_cap * sizeof(Tether_SparseMap));
      g_registry.dense_arrays = (Tether_DenseArray *)realloc(g_registry.dense_arrays, new_cap * sizeof(Tether_DenseArray));
      
      /* Initialize the newly allocated memory */
      memset(g_registry.sparse_maps + old_cap, 0, (new_cap - old_cap) * sizeof(Tether_SparseMap));
      memset(g_registry.dense_arrays + old_cap, 0, (new_cap - old_cap) * sizeof(Tether_DenseArray));
      
      g_registry.component_type_capacity = new_cap;
  }

  Tether_DenseArray *dense = &g_registry.dense_arrays[component_id];
  dense->element_size = element_size;
}

uint32_t tether_ecs_register_component_dynamic(size_t element_size) {
  uint32_t id = g_registry.next_custom_component_id++;
  tether_ecs_register_component_static(id, element_size);
  return id;
}

void tether_ecs_detach_entity(Tether_GUID entity) {
  if (!tether_ecs_is_valid(entity)) return;

  Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
  if (!h || h->parent == 0) return; // Not attached to anything

  Tether_Hierarchy* p = (Tether_Hierarchy*)tether_ecs_get_component(h->parent, TETHER_COMPONENT_HIERARCHY);
  if (p) {
      if (p->first_child == entity) {
          p->first_child = h->next_sibling;
      }
      if (p->last_child == entity) {
          p->last_child = h->prev_sibling;
      }
      if (p->child_count > 0) {
          p->child_count--;
      }
      /* Flag parent for hierarchy sync so renderer updates the scene graph */
      tether_ecs_add_component(h->parent, TETHER_COMPONENT_DIRTY_HIERARCHY);
  }

  // Stitch neighbors together O(1)
  if (h->prev_sibling != 0) {
      Tether_Hierarchy* prev = (Tether_Hierarchy*)tether_ecs_get_component(h->prev_sibling, TETHER_COMPONENT_HIERARCHY);
      if (prev) prev->next_sibling = h->next_sibling;
  }
  if (h->next_sibling != 0) {
      Tether_Hierarchy* next = (Tether_Hierarchy*)tether_ecs_get_component(h->next_sibling, TETHER_COMPONENT_HIERARCHY);
      if (next) next->prev_sibling = h->prev_sibling;
  }

  // Clear own references
  h->parent = 0;
  h->prev_sibling = 0;
  h->next_sibling = 0;
}

void tether_ecs_attach_entity(Tether_GUID parent, Tether_GUID entity) {
    if (!tether_ecs_is_valid(parent) || !tether_ecs_is_valid(entity)) return;

    if (tether_ecs_get_component(parent, TETHER_COMPONENT_IS_LEAF)) return;

    /* Make sure it's completely detached from any old parent first! */
  tether_ecs_detach_entity(entity);

  Tether_Hierarchy* p = (Tether_Hierarchy*)tether_ecs_get_component(parent, TETHER_COMPONENT_HIERARCHY);
  Tether_Hierarchy* c = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
  
  if (!p || !c) return; // Programmer MUST add components explicitly!

  c->parent = parent;
  c->prev_sibling = 0;
  c->next_sibling = 0;

  if (p->first_child == 0) {
      p->first_child = entity;
      p->last_child = entity;
  } else {
      // O(1) Instant Append using last_child!
      Tether_GUID old_last = p->last_child;
      Tether_Hierarchy* old_last_h = (Tether_Hierarchy*)tether_ecs_get_component(old_last, TETHER_COMPONENT_HIERARCHY);
      
      if (old_last_h) {
          old_last_h->next_sibling = entity;
          c->prev_sibling = old_last;
      }
      p->last_child = entity;
  }

  p->child_count++;
  /* Flag new parent for hierarchy sync so renderer updates the scene graph */
  tether_ecs_add_component(parent, TETHER_COMPONENT_DIRTY_HIERARCHY);
}

void tether_ecs_bring_to_front(Tether_GUID entity) {
    if (!tether_ecs_is_valid(entity)) return;

    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    if (!h || h->parent == 0) return; // Not attached

    Tether_GUID parent = h->parent;
    tether_ecs_detach_entity(entity);
    tether_ecs_attach_entity(parent, entity);
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
      (uint32_t)component_id >= g_registry.component_type_capacity)
    return NULL;

  Tether_DenseArray *dense = &g_registry.dense_arrays[component_id];

  uint32_t entity_index = tether_ecs_get_index(entity);
  ensure_sparse_capacity(component_id, entity_index + 1);

  Tether_SparseMap *sparse = &g_registry.sparse_maps[component_id];

  /* If it already has the component, return the existing one */
  if (sparse->dense_indices[entity_index] != TETHER_SPARSE_INVALID_INDEX) {
    uint32_t dense_idx = sparse->dense_indices[entity_index];
    if (dense->element_size > 0) {
        return (uint8_t *)dense->data + (dense_idx * dense->element_size);
    }
    return (void*)1;
  }

  /* Add new component */
  if (dense->count >= dense->capacity) {
    dense->capacity =
        dense->capacity == 0 ? INITIAL_DENSE_CAPACITY : dense->capacity * 2;
    if (dense->element_size > 0) {
        dense->data = realloc(dense->data, dense->capacity * dense->element_size);
    }
    dense->entity_map = (Tether_GUID *)realloc(
        dense->entity_map, dense->capacity * sizeof(Tether_GUID));
  }

  uint32_t dense_idx = dense->count++;
  sparse->dense_indices[entity_index] = dense_idx;
  dense->entity_map[dense_idx] = entity;

  void *ptr = NULL;
  if (dense->element_size > 0) {
      ptr = (uint8_t *)dense->data + (dense_idx * dense->element_size);
      memset(ptr, 0, dense->element_size);
  } else {
      ptr = (void*)1;
  }
  return ptr;
}

void *tether_ecs_get_component(Tether_GUID entity, int component_id) {
  if (!tether_ecs_is_valid(entity) || component_id < 0 ||
      (uint32_t)component_id >= g_registry.component_type_capacity)
    return NULL;

  uint32_t entity_index = tether_ecs_get_index(entity);
  Tether_SparseMap *sparse = &g_registry.sparse_maps[component_id];
  if (entity_index >= sparse->capacity ||
      sparse->dense_indices[entity_index] == TETHER_SPARSE_INVALID_INDEX) {
    return NULL;
  }

  Tether_DenseArray *dense = &g_registry.dense_arrays[component_id];
  uint32_t dense_idx = sparse->dense_indices[entity_index];
  
  if (dense->element_size == 0) {
      return (void*)1; /* Return a non-null dummy pointer for tag components */
  }
  return (uint8_t *)dense->data + (dense_idx * dense->element_size);
}

void tether_ecs_remove_component(Tether_GUID entity, int component_id) {
  if (!tether_ecs_is_valid(entity) || component_id < 0 ||
      (uint32_t)component_id >= g_registry.component_type_capacity)
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
    if (dense->element_size > 0) {
        void *dst = (uint8_t *)dense->data + (dense_idx_to_remove * dense->element_size);
        void *src = (uint8_t *)dense->data + (last_dense_idx * dense->element_size);
        memcpy(dst, src, dense->element_size);
    }

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
  if (component_id < 0 || (uint32_t)component_id >= g_registry.component_type_capacity)
    return NULL;
  return &g_registry.dense_arrays[component_id];
}

const char* tether_ecs_get_text_string(Tether_GUID entity) {
    /* Fast check from smallest to largest */
    Tether_TextWord* word = (Tether_TextWord*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_WORD);
    if (word) return word->data;
    
    Tether_TextLabel* label = (Tether_TextLabel*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_LABEL);
    if (label) return label->data;
    
    Tether_TextParagraph* paragraph = (Tether_TextParagraph*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_PARAGRAPH);
    if (paragraph) return paragraph->data;
    
    Tether_TextDynamic* dynamic = (Tether_TextDynamic*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_DYNAMIC);
    if (dynamic) return dynamic->data;
    
    return NULL;
}

  bool tether_ecs_set_text_string(Tether_GUID entity, const char* string) {
    if (!tether_ecs_is_valid(entity) || !string) return false;

    size_t length = strlen(string);
    Tether_TextDynamic* dynamic = (Tether_TextDynamic*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_DYNAMIC);
    if (dynamic && dynamic->data) {
      free(dynamic->data);
      dynamic->data = NULL;
    }

    tether_ecs_remove_component(entity, TETHER_COMPONENT_TEXT_WORD);
    tether_ecs_remove_component(entity, TETHER_COMPONENT_TEXT_LABEL);
    tether_ecs_remove_component(entity, TETHER_COMPONENT_TEXT_PARAGRAPH);
    tether_ecs_remove_component(entity, TETHER_COMPONENT_TEXT_DYNAMIC);

    if (length <= 31) {
      Tether_TextWord* text = (Tether_TextWord*)tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT_WORD);
      if (!text) return false;
      memcpy(text->data, string, length + 1);
    } else if (length <= 127) {
      Tether_TextLabel* text = (Tether_TextLabel*)tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT_LABEL);
      if (!text) return false;
      memcpy(text->data, string, length + 1);
    } else if (length <= 511) {
      Tether_TextParagraph* text = (Tether_TextParagraph*)tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT_PARAGRAPH);
      if (!text) return false;
      memcpy(text->data, string, length + 1);
    } else {
      Tether_TextDynamic* text = (Tether_TextDynamic*)tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT_DYNAMIC);
      if (!text) return false;
      text->capacity = (uint32_t)(length + 1) * 2;
      text->data = (char*)malloc(text->capacity);
      if (!text->data) {
        tether_ecs_remove_component(entity, TETHER_COMPONENT_TEXT_DYNAMIC);
        return false;
      }
      memcpy(text->data, string, length + 1);
      text->length = (uint32_t)length;
    }

    return true;
  }

Tether_GUID tether_ecs_find_by_id(const char* id) {
    if (!id || id[0] == '\0') return TETHER_INVALID_GUID;
    
    Tether_DenseArray* ids = tether_ecs_get_dense_array(TETHER_COMPONENT_ID);
    if (!ids) return TETHER_INVALID_GUID;
    
    for (uint32_t i = 0; i < ids->count; i++) {
        Tether_Id* node = (Tether_Id*)((uint8_t*)ids->data + (i * ids->element_size));
        if (strcmp(node->id, id) == 0) {
            return ids->entity_map[i];
        }
    }
    return TETHER_INVALID_GUID;
}

/* --- Font Registry API --- */

uint32_t tether_font_register(const char* name, const char* path) {
    if (!name || !path) return 0;
    
    /* Check if already registered */
    for (uint32_t i = 1; i <= g_registry.font_registry.count; ++i) {
        if (strcmp(g_registry.font_registry.names[i], name) == 0) {
            return i;
        }
    }
    
    if (g_registry.font_registry.count >= 31) {
        return 0; /* Full */
    }
    
    g_registry.font_registry.count++;
    uint32_t id = g_registry.font_registry.count;
    
    strncpy(g_registry.font_registry.names[id], name, 63);
    g_registry.font_registry.names[id][63] = '\0';
    
    strncpy(g_registry.font_registry.paths[id], path, 255);
    g_registry.font_registry.paths[id][255] = '\0';
    
    return id;
}

const char* tether_font_get_path(uint32_t font_id) {
    if (font_id == 0 || font_id > g_registry.font_registry.count) return NULL;
    return g_registry.font_registry.paths[font_id];
}

const char* tether_font_get_name(uint32_t font_id) {
    if (font_id == 0 || font_id > g_registry.font_registry.count) return NULL;
    return g_registry.font_registry.names[font_id];
}
