#ifndef TETHER_HASHMAP_H
#define TETHER_HASHMAP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * String -> U64 Map (Robin Hood)
 * ========================================================================= */
typedef struct {
    char* key;
    uint64_t value;
    uint32_t psl;
    bool occupied;
} Tether_HashEntry;

typedef struct {
    Tether_HashEntry* entries;
    uint32_t count;
    uint32_t capacity;
} Tether_HashMap;

void tether_hashmap_init(Tether_HashMap* map, uint32_t initial_capacity);
void tether_hashmap_term(Tether_HashMap* map);
void tether_hashmap_set(Tether_HashMap* map, const char* key, uint64_t value);
bool tether_hashmap_get(Tether_HashMap* map, const char* key, uint64_t* out_value);
void tether_hashmap_remove(Tether_HashMap* map, const char* key);

/* =========================================================================
 * U64 -> String Map (Robin Hood)
 * ========================================================================= */
typedef struct {
    uint64_t key;
    char* value;
    uint32_t psl;
    bool occupied;
} Tether_U64MapEntry;

typedef struct {
    Tether_U64MapEntry* entries;
    uint32_t count;
    uint32_t capacity;
} Tether_U64Map;

void tether_u64map_init(Tether_U64Map* map, uint32_t initial_capacity);
void tether_u64map_term(Tether_U64Map* map);
void tether_u64map_set(Tether_U64Map* map, uint64_t key, const char* value);
bool tether_u64map_get(Tether_U64Map* map, uint64_t key, const char** out_value);
void tether_u64map_remove(Tether_U64Map* map, uint64_t key);

#ifdef __cplusplus
}
#endif

#endif // TETHER_HASHMAP_H
