#include "tether/parsers/tether_yaml.h"
#include "tether/core/tether_components.h"
#include <yaml.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Tether_GUID create_panel(Tether_GUID parent) {
    Tether_GUID entity = tether_ecs_create_entity();
    printf("[YAML] Created Panel: %llu (Parent: %llu)\n", (unsigned long long)entity, (unsigned long long)parent); fflush(stdout);
    
    Tether_Transform* t = (Tether_Transform*)tether_ecs_add_component(entity, TETHER_COMPONENT_TRANSFORM);
    t->x = 0; t->y = 0; t->width = 100; t->height = 100;
    
    Tether_Color* c = (Tether_Color*)tether_ecs_add_component(entity, TETHER_COMPONENT_COLOR);
    c->r = 255; c->g = 255; c->b = 255; c->a = 255;

    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_add_component(entity, TETHER_COMPONENT_HIERARCHY);
    h->parent = parent;
    h->first_child = TETHER_INVALID_GUID;
    h->next_sibling = TETHER_INVALID_GUID;
    h->child_count = 0;
    
    if (parent != TETHER_INVALID_GUID) {
        Tether_Hierarchy* ph = (Tether_Hierarchy*)tether_ecs_get_component(parent, TETHER_COMPONENT_HIERARCHY);
        if (ph) {
            ph->child_count++;
            if (ph->first_child == TETHER_INVALID_GUID) {
                ph->first_child = entity;
            } else {
                Tether_GUID sibling = ph->first_child;
                while (1) {
                    Tether_Hierarchy* sh = (Tether_Hierarchy*)tether_ecs_get_component(sibling, TETHER_COMPONENT_HIERARCHY);
                    if (sh->next_sibling == TETHER_INVALID_GUID) {
                        sh->next_sibling = entity;
                        break;
                    }
                    sibling = sh->next_sibling;
                }
            }
        }
    }

    return entity;
}

Tether_GUID tether_yaml_load(const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        printf("[YAML] Failed to open %s\n", filepath);
        return TETHER_INVALID_GUID;
    }

    yaml_parser_t parser;
    yaml_event_t event;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, file);

    Tether_GUID root = TETHER_INVALID_GUID;
    
    /* Stack for hierarchy */
    Tether_GUID entity_stack[64];
    int stack_idx = -1;
    
    /* State tracking */
    enum { STATE_NONE, STATE_TRANSFORM, STATE_COLOR, STATE_CHILDREN } state = STATE_NONE;
    int array_idx = 0;
    
    int mapping_depth = 0;

    while (1) {
        if (!yaml_parser_parse(&parser, &event)) {
            printf("[YAML] Parser error\n");
            break;
        }

        if (event.type == YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        switch (event.type) {
            case YAML_MAPPING_START_EVENT:
                mapping_depth++;
                break;
                
            case YAML_MAPPING_END_EVENT:
                mapping_depth--;
                if (mapping_depth % 2 == 0 && stack_idx >= 0) {
                    /* Finished parsing a Panel's properties */
                    stack_idx--;
                }
                break;
                
            case YAML_SEQUENCE_START_EVENT:
                if (state == STATE_CHILDREN) {
                    /* We are entering the children list, the mapping depth will increase on the next item */
                }
                break;
                
            case YAML_SEQUENCE_END_EVENT:
                if (state == STATE_TRANSFORM || state == STATE_COLOR) {
                    state = STATE_NONE;
                }
                break;

            case YAML_SCALAR_EVENT: {
                const char* value = (const char*)event.data.scalar.value;
                
                if (strcmp(value, "Panel") == 0) {
                    Tether_GUID parent = stack_idx >= 0 ? entity_stack[stack_idx] : TETHER_INVALID_GUID;
                    Tether_GUID new_ent = create_panel(parent);
                    if (root == TETHER_INVALID_GUID) root = new_ent;
                    entity_stack[++stack_idx] = new_ent;
                    state = STATE_NONE;
                } 
                else if (strcmp(value, "transform") == 0) {
                    state = STATE_TRANSFORM;
                    array_idx = 0;
                }
                else if (strcmp(value, "color") == 0) {
                    state = STATE_COLOR;
                    array_idx = 0;
                }
                else if (strcmp(value, "children") == 0) {
                    state = STATE_CHILDREN;
                }
                else {
                    /* Read Array Value */
                    if (state == STATE_TRANSFORM && stack_idx >= 0) {
                        Tether_Transform* t = (Tether_Transform*)tether_ecs_get_component(entity_stack[stack_idx], TETHER_COMPONENT_TRANSFORM);
                        float v = strtof(value, NULL);
                        if (array_idx == 0) t->x = v;
                        else if (array_idx == 1) t->y = v;
                        else if (array_idx == 2) t->width = v;
                        else if (array_idx == 3) t->height = v;
                        printf("[YAML] Parsed transform[%d] = %f\n", array_idx, v); fflush(stdout);
                        array_idx++;
                    }
                    else if (state == STATE_COLOR && stack_idx >= 0) {
                        Tether_Color* c = (Tether_Color*)tether_ecs_get_component(entity_stack[stack_idx], TETHER_COMPONENT_COLOR);
                        int v = atoi(value);
                        if (array_idx == 0) c->r = v;
                        else if (array_idx == 1) c->g = v;
                        else if (array_idx == 2) c->b = v;
                        else if (array_idx == 3) c->a = v;
                        printf("[YAML] Parsed color[%d] = %d\n", array_idx, v); fflush(stdout);
                        array_idx++;
                    }
                }
                break;
            }
            default:
                break;
        }

        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    fclose(file);
    return root;
}
