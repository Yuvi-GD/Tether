#include "tether/core/tether_registry.h"
#include "tether/utils/tether_hashmap.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* =========================================================================
 * Data Structures
 * ========================================================================= */
typedef struct {
    char name[64];
    char path[256];
} Tether_FontEntry;

typedef struct {
    Tether_FontEntry entries[32];
    uint32_t count;
} Tether_FontRegistry;

/* Global state */
static Tether_HashMap g_widget_registry = {0};
static Tether_FontRegistry g_font_registry = {0};

static Tether_HashMap g_alias_to_entity = {0};
static Tether_U64Map g_entity_to_alias = {0};

/* =========================================================================
 * Init & Term
 * ========================================================================= */
void tether_registry_init(void) {
    /* Init Widget Registry */
    tether_hashmap_init(&g_widget_registry, 16);

    /* Init Font Registry */
    memset(g_font_registry.entries, 0, sizeof(g_font_registry.entries));
    g_font_registry.count = 0;

    /* Init ID Registry */
    tether_hashmap_init(&g_alias_to_entity, 16);
    tether_u64map_init(&g_entity_to_alias, 16);
}

void tether_registry_term(void) {
    tether_hashmap_term(&g_widget_registry);
    tether_hashmap_term(&g_alias_to_entity);
    tether_u64map_term(&g_entity_to_alias);
    
    memset(g_font_registry.entries, 0, sizeof(g_font_registry.entries));
    g_font_registry.count = 0;
}

/* =========================================================================
 * Widget Registry
 * ========================================================================= */
void tether_register_widget(const char* name, Tether_WidgetCreateFn create_fn) {
    if (!name || !create_fn) return;
    tether_hashmap_set(&g_widget_registry, name, (uint64_t)create_fn);
}

Tether_GUID tether_create_widget(const char* name, Tether_GUID parent) {
    if (!name) return TETHER_INVALID_GUID;

    uint64_t fn_ptr = 0;
    if (tether_hashmap_get(&g_widget_registry, name, &fn_ptr)) {
        Tether_WidgetCreateFn create_fn = (Tether_WidgetCreateFn)fn_ptr;
        return create_fn(parent);
    }

    printf("[Tether Registry] Warning: Unrecognized widget '%s'\n", name);
    return TETHER_INVALID_GUID;
}

/* =========================================================================
 * ID Registry
 * ========================================================================= */
void tether_registry_alias_entity(Tether_GUID entity, const char* alias) {
    if (!tether_ecs_is_valid(entity) || !alias || alias[0] == '\0') return;

    /* Remove existing if any */
    tether_registry_remove_alias(entity);
    Tether_GUID old_entity = tether_registry_find_entity(alias);
    if (tether_ecs_is_valid(old_entity)) {
        tether_registry_remove_alias(old_entity);
    }

    tether_hashmap_set(&g_alias_to_entity, alias, (uint64_t)entity);
    tether_u64map_set(&g_entity_to_alias, (uint64_t)entity, alias);
}

const char* tether_registry_get_alias(Tether_GUID entity) {
    if (!tether_ecs_is_valid(entity)) return NULL;
    
    const char* alias = NULL;
    tether_u64map_get(&g_entity_to_alias, (uint64_t)entity, &alias);
    return alias;
}

Tether_GUID tether_registry_find_entity(const char* alias) {
    if (!alias || alias[0] == '\0') return TETHER_INVALID_GUID;
    
    uint64_t entity = 0;
    if (tether_hashmap_get(&g_alias_to_entity, alias, &entity)) {
        return (Tether_GUID)entity;
    }
    return TETHER_INVALID_GUID;
}

void tether_registry_remove_alias(Tether_GUID entity) {
    if (!tether_ecs_is_valid(entity)) return;

    const char* alias = NULL;
    if (tether_u64map_get(&g_entity_to_alias, (uint64_t)entity, &alias)) {
        /* Remove from reverse map using the string we found */
        tether_hashmap_remove(&g_alias_to_entity, alias);
        tether_u64map_remove(&g_entity_to_alias, (uint64_t)entity);
    }
}

/* =========================================================================
 * Font Registry
 * ========================================================================= */
uint32_t tether_font_register(const char* name, const char* path) {
    if (!name || !path) return 0;

    for (uint32_t i = 0; i < g_font_registry.count; ++i) {
        if (strcmp(g_font_registry.entries[i].name, name) == 0) {
            return i + 1;
        }
    }

    if (g_font_registry.count >= 31) {
        return 0;
    }

    uint32_t id = g_font_registry.count + 1;
    strncpy(g_font_registry.entries[g_font_registry.count].name, name, 63);
    g_font_registry.entries[g_font_registry.count].name[63] = '\0';
    strncpy(g_font_registry.entries[g_font_registry.count].path, path, 255);
    g_font_registry.entries[g_font_registry.count].path[255] = '\0';
    g_font_registry.count++;
    return id;
}

const char* tether_font_get_path(uint32_t font_id) {
    if (font_id == 0 || font_id > g_font_registry.count) return NULL;
    return g_font_registry.entries[font_id - 1].path;
}

const char* tether_font_get_name(uint32_t font_id) {
    if (font_id == 0 || font_id > g_font_registry.count) return NULL;
    return g_font_registry.entries[font_id - 1].name;
}
