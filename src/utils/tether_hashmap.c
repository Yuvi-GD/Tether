#include "tether/utils/tether_hashmap.h"
#include <stdlib.h>
#include <string.h>

#define MAX_LOAD_FACTOR 0.75f

static uint32_t hash_string(const char* str) {
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= (uint8_t)(*str++);
        hash *= 16777619u;
    }
    return hash;
}

static uint32_t hash_u64(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return (uint32_t)(x ^ (x >> 32));
}

/* =========================================================================
 * String -> U64 Map
 * ========================================================================= */

void tether_hashmap_init(Tether_HashMap* map, uint32_t initial_capacity) {
    if (initial_capacity < 8) initial_capacity = 8;
    /* Ensure power of two */
    uint32_t cap = 1;
    while (cap < initial_capacity) cap <<= 1;
    
    map->capacity = cap;
    map->count = 0;
    map->entries = (Tether_HashEntry*)calloc(cap, sizeof(Tether_HashEntry));
}

void tether_hashmap_term(Tether_HashMap* map) {
    if (!map || !map->entries) return;
    for (uint32_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].occupied && map->entries[i].key) {
            free(map->entries[i].key);
        }
    }
    free(map->entries);
    map->entries = NULL;
    map->count = 0;
    map->capacity = 0;
}

static void hashmap_insert(Tether_HashEntry* entries, uint32_t capacity, Tether_HashEntry entry) {
    uint32_t index = hash_string(entry.key) & (capacity - 1);
    while (true) {
        if (!entries[index].occupied) {
            entries[index] = entry;
            return;
        }
        if (entries[index].psl < entry.psl) {
            Tether_HashEntry temp = entries[index];
            entries[index] = entry;
            entry = temp;
        }
        index = (index + 1) & (capacity - 1);
        entry.psl++;
    }
}

static void hashmap_resize(Tether_HashMap* map) {
    uint32_t new_cap = map->capacity * 2;
    Tether_HashEntry* new_entries = (Tether_HashEntry*)calloc(new_cap, sizeof(Tether_HashEntry));
    
    for (uint32_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].occupied) {
            Tether_HashEntry entry = map->entries[i];
            entry.psl = 0;
            hashmap_insert(new_entries, new_cap, entry);
        }
    }
    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_cap;
}

void tether_hashmap_set(Tether_HashMap* map, const char* key, uint64_t value) {
    if (!map || !map->entries || !key) return;
    
    /* Update if exists */
    uint32_t index = hash_string(key) & (map->capacity - 1);
    uint32_t psl = 0;
    while (map->entries[index].occupied) {
        if (strcmp(map->entries[index].key, key) == 0) {
            map->entries[index].value = value;
            return;
        }
        if (map->entries[index].psl < psl) break;
        index = (index + 1) & (map->capacity - 1);
        psl++;
    }
    
    if ((float)(map->count + 1) / map->capacity > MAX_LOAD_FACTOR) {
        hashmap_resize(map);
    }
    
    Tether_HashEntry new_entry = {0};
    new_entry.key = strdup(key);
    new_entry.value = value;
    new_entry.psl = 0;
    new_entry.occupied = true;
    
    hashmap_insert(map->entries, map->capacity, new_entry);
    map->count++;
}

bool tether_hashmap_get(Tether_HashMap* map, const char* key, uint64_t* out_value) {
    if (!map || !map->entries || !key) return false;
    uint32_t index = hash_string(key) & (map->capacity - 1);
    uint32_t psl = 0;
    while (map->entries[index].occupied) {
        if (strcmp(map->entries[index].key, key) == 0) {
            if (out_value) *out_value = map->entries[index].value;
            return true;
        }
        if (map->entries[index].psl < psl) break;
        index = (index + 1) & (map->capacity - 1);
        psl++;
    }
    return false;
}

void tether_hashmap_remove(Tether_HashMap* map, const char* key) {
    if (!map || !map->entries || !key) return;
    uint32_t index = hash_string(key) & (map->capacity - 1);
    uint32_t psl = 0;
    while (map->entries[index].occupied) {
        if (strcmp(map->entries[index].key, key) == 0) {
            free(map->entries[index].key);
            map->entries[index].occupied = false;
            map->count--;
            
            /* Backward shift */
            uint32_t next = (index + 1) & (map->capacity - 1);
            while (map->entries[next].occupied && map->entries[next].psl > 0) {
                map->entries[index] = map->entries[next];
                map->entries[index].psl--;
                map->entries[next].occupied = false;
                index = next;
                next = (index + 1) & (map->capacity - 1);
            }
            return;
        }
        if (map->entries[index].psl < psl) break;
        index = (index + 1) & (map->capacity - 1);
        psl++;
    }
}

/* =========================================================================
 * U64 -> String Map
 * ========================================================================= */

void tether_u64map_init(Tether_U64Map* map, uint32_t initial_capacity) {
    if (initial_capacity < 8) initial_capacity = 8;
    uint32_t cap = 1;
    while (cap < initial_capacity) cap <<= 1;
    
    map->capacity = cap;
    map->count = 0;
    map->entries = (Tether_U64MapEntry*)calloc(cap, sizeof(Tether_U64MapEntry));
}

void tether_u64map_term(Tether_U64Map* map) {
    if (!map || !map->entries) return;
    for (uint32_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].occupied && map->entries[i].value) {
            free(map->entries[i].value);
        }
    }
    free(map->entries);
    map->entries = NULL;
    map->count = 0;
    map->capacity = 0;
}

static void u64map_insert(Tether_U64MapEntry* entries, uint32_t capacity, Tether_U64MapEntry entry) {
    uint32_t index = hash_u64(entry.key) & (capacity - 1);
    while (true) {
        if (!entries[index].occupied) {
            entries[index] = entry;
            return;
        }
        if (entries[index].psl < entry.psl) {
            Tether_U64MapEntry temp = entries[index];
            entries[index] = entry;
            entry = temp;
        }
        index = (index + 1) & (capacity - 1);
        entry.psl++;
    }
}

static void u64map_resize(Tether_U64Map* map) {
    uint32_t new_cap = map->capacity * 2;
    Tether_U64MapEntry* new_entries = (Tether_U64MapEntry*)calloc(new_cap, sizeof(Tether_U64MapEntry));
    
    for (uint32_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].occupied) {
            Tether_U64MapEntry entry = map->entries[i];
            entry.psl = 0;
            u64map_insert(new_entries, new_cap, entry);
        }
    }
    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_cap;
}

void tether_u64map_set(Tether_U64Map* map, uint64_t key, const char* value) {
    if (!map || !map->entries || !value) return;
    
    /* Update if exists */
    uint32_t index = hash_u64(key) & (map->capacity - 1);
    uint32_t psl = 0;
    while (map->entries[index].occupied) {
        if (map->entries[index].key == key) {
            free(map->entries[index].value);
            map->entries[index].value = strdup(value);
            return;
        }
        if (map->entries[index].psl < psl) break;
        index = (index + 1) & (map->capacity - 1);
        psl++;
    }
    
    if ((float)(map->count + 1) / map->capacity > MAX_LOAD_FACTOR) {
        u64map_resize(map);
    }
    
    Tether_U64MapEntry new_entry = {0};
    new_entry.key = key;
    new_entry.value = strdup(value);
    new_entry.psl = 0;
    new_entry.occupied = true;
    
    u64map_insert(map->entries, map->capacity, new_entry);
    map->count++;
}

bool tether_u64map_get(Tether_U64Map* map, uint64_t key, const char** out_value) {
    if (!map || !map->entries) return false;
    uint32_t index = hash_u64(key) & (map->capacity - 1);
    uint32_t psl = 0;
    while (map->entries[index].occupied) {
        if (map->entries[index].key == key) {
            if (out_value) *out_value = map->entries[index].value;
            return true;
        }
        if (map->entries[index].psl < psl) break;
        index = (index + 1) & (map->capacity - 1);
        psl++;
    }
    return false;
}

void tether_u64map_remove(Tether_U64Map* map, uint64_t key) {
    if (!map || !map->entries) return;
    uint32_t index = hash_u64(key) & (map->capacity - 1);
    uint32_t psl = 0;
    while (map->entries[index].occupied) {
        if (map->entries[index].key == key) {
            free(map->entries[index].value);
            map->entries[index].occupied = false;
            map->count--;
            
            /* Backward shift */
            uint32_t next = (index + 1) & (map->capacity - 1);
            while (map->entries[next].occupied && map->entries[next].psl > 0) {
                map->entries[index] = map->entries[next];
                map->entries[index].psl--;
                map->entries[next].occupied = false;
                index = next;
                next = (index + 1) & (map->capacity - 1);
            }
            return;
        }
        if (map->entries[index].psl < psl) break;
        index = (index + 1) & (map->capacity - 1);
        psl++;
    }
}
