#include "tether/core/tether_registry.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    char name[64];
    Tether_WidgetCreateFn create_fn;
} Tether_WidgetEntry;

typedef struct {
    Tether_WidgetEntry* entries;
    uint32_t count;
    uint32_t capacity;
} Tether_WidgetRegistry;

static Tether_WidgetRegistry g_widget_registry = {0};

void tether_registry_init(void) {
    g_widget_registry.capacity = 16;
    g_widget_registry.count = 0;
    g_widget_registry.entries = (Tether_WidgetEntry*)malloc(g_widget_registry.capacity * sizeof(Tether_WidgetEntry));
}

void tether_registry_term(void) {
    if (g_widget_registry.entries) {
        free(g_widget_registry.entries);
    }
    g_widget_registry.entries = NULL;
    g_widget_registry.count = 0;
    g_widget_registry.capacity = 0;
}

void tether_register_widget(const char* name, Tether_WidgetCreateFn create_fn) {
    if (!name || !create_fn) return;

    /* Check for duplicate */
    for (uint32_t i = 0; i < g_widget_registry.count; i++) {
        if (strcmp(g_widget_registry.entries[i].name, name) == 0) {
            /* Overwrite existing */
            g_widget_registry.entries[i].create_fn = create_fn;
            return;
        }
    }

    /* Expand capacity if necessary */
    if (g_widget_registry.count >= g_widget_registry.capacity) {
        g_widget_registry.capacity *= 2;
        g_widget_registry.entries = (Tether_WidgetEntry*)realloc(
            g_widget_registry.entries, 
            g_widget_registry.capacity * sizeof(Tether_WidgetEntry)
        );
    }

    /* Add new widget */
    strncpy(g_widget_registry.entries[g_widget_registry.count].name, name, 63);
    g_widget_registry.entries[g_widget_registry.count].name[63] = '\0';
    g_widget_registry.entries[g_widget_registry.count].create_fn = create_fn;
    g_widget_registry.count++;
}

Tether_GUID tether_create_widget(const char* name, Tether_GUID parent) {
    if (!name) return TETHER_INVALID_GUID;

    for (uint32_t i = 0; i < g_widget_registry.count; i++) {
        if (strcmp(g_widget_registry.entries[i].name, name) == 0) {
            return g_widget_registry.entries[i].create_fn(parent);
        }
    }

    printf("[Tether Registry] Warning: Unrecognized widget '%s'\n", name);
    return TETHER_INVALID_GUID;
}
