#include "parsers/tether_yaml_ast.h"
#include <yaml.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Tether_ASTNode* create_node(Tether_ASTNodeType type) {
    Tether_ASTNode* node = (Tether_ASTNode*)calloc(1, sizeof(Tether_ASTNode));
    node->type = type;
    return node;
}

static void add_child(Tether_ASTNode* parent, Tether_ASTNode* child, Tether_ASTNode* key) {
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        parent->children = (Tether_ASTNode**)realloc(parent->children, parent->child_capacity * sizeof(Tether_ASTNode*));
        if (parent->type == TETHER_AST_MAPPING) {
            parent->keys = (Tether_ASTNode**)realloc(parent->keys, parent->child_capacity * sizeof(Tether_ASTNode*));
        }
    }
    parent->children[parent->child_count] = child;
    if (parent->type == TETHER_AST_MAPPING) {
        parent->keys[parent->child_count] = key;
    }
    parent->child_count++;
}

void tether_ast_free(Tether_ASTNode* node) {
    if (!node) return;
    if (node->scalar_value) free(node->scalar_value);
    
    for (uint32_t i = 0; i < node->child_count; i++) {
        tether_ast_free(node->children[i]);
        if (node->type == TETHER_AST_MAPPING && node->keys[i]) {
            tether_ast_free(node->keys[i]);
        }
    }
    if (node->children) free(node->children);
    if (node->keys) free(node->keys);
    free(node);
}

Tether_ASTNode* tether_ast_parse_file(const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file) return NULL;

    yaml_parser_t parser;
    yaml_event_t event;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, file);

    Tether_ASTNode* root = NULL;
    Tether_ASTNode* stack[64];
    int stack_idx = -1;
    Tether_ASTNode* current_key = NULL;

    while (yaml_parser_parse(&parser, &event)) {
        if (event.type == YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        switch (event.type) {
            case YAML_SCALAR_EVENT: {
                Tether_ASTNode* node = create_node(TETHER_AST_SCALAR);
                node->scalar_value = strdup((const char*)event.data.scalar.value);
                
                if (stack_idx >= 0) {
                    Tether_ASTNode* parent = stack[stack_idx];
                    if (parent->type == TETHER_AST_MAPPING) {
                        if (!current_key) {
                            current_key = node;
                        } else {
                            add_child(parent, node, current_key);
                            current_key = NULL;
                        }
                    } else if (parent->type == TETHER_AST_SEQUENCE) {
                        add_child(parent, node, NULL);
                    }
                } else {
                    root = node;
                }
                break;
            }
            case YAML_SEQUENCE_START_EVENT:
            case YAML_MAPPING_START_EVENT: {
                Tether_ASTNodeType type = (event.type == YAML_SEQUENCE_START_EVENT) ? TETHER_AST_SEQUENCE : TETHER_AST_MAPPING;
                Tether_ASTNode* node = create_node(type);
                
                if (stack_idx >= 0) {
                    Tether_ASTNode* parent = stack[stack_idx];
                    if (parent->type == TETHER_AST_MAPPING) {
                        add_child(parent, node, current_key);
                        current_key = NULL;
                    } else if (parent->type == TETHER_AST_SEQUENCE) {
                        add_child(parent, node, NULL);
                    }
                } else {
                    root = node;
                }
                stack[++stack_idx] = node;
                break;
            }
            case YAML_SEQUENCE_END_EVENT:
            case YAML_MAPPING_END_EVENT: {
                if (stack_idx >= 0) {
                    stack_idx--;
                }
                break;
            }
            default: break;
        }
        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    fclose(file);
    return root;
}

Tether_ASTEnvironment* tether_ast_env_create(void) {
    Tether_ASTEnvironment* env = (Tether_ASTEnvironment*)calloc(1, sizeof(Tether_ASTEnvironment));
    return env;
}

void tether_ast_env_set(Tether_ASTEnvironment* env, const char* key, const char* value) {
    if (!env || !key || !value) return;
    
    for (uint32_t i = 0; i < env->count; i++) {
        if (strcmp(env->keys[i], key) == 0) {
            free(env->values[i]);
            env->values[i] = strdup(value);
            return;
        }
    }
    
    if (env->count >= env->capacity) {
        env->capacity = env->capacity == 0 ? 4 : env->capacity * 2;
        env->keys = (char**)realloc(env->keys, env->capacity * sizeof(char*));
        env->values = (char**)realloc(env->values, env->capacity * sizeof(char*));
    }
    env->keys[env->count] = strdup(key);
    env->values[env->count] = strdup(value);
    env->count++;
}

const char* tether_ast_env_get(Tether_ASTEnvironment* env, const char* key) {
    if (!env || !key) return NULL;
    for (uint32_t i = 0; i < env->count; i++) {
        if (strcmp(env->keys[i], key) == 0) {
            return env->values[i];
        }
    }
    return NULL;
}

void tether_ast_env_set_slot(Tether_ASTEnvironment* env, const char* key, Tether_ASTNode* node) {
    if (!env || !key) return;
    
    for (uint32_t i = 0; i < env->slot_count; i++) {
        if (strcmp(env->slot_keys[i], key) == 0) {
            env->slot_values[i] = node; /* Weak ref to AST node */
            return;
        }
    }
    
    if (env->slot_count >= env->slot_capacity) {
        env->slot_capacity = env->slot_capacity == 0 ? 4 : env->slot_capacity * 2;
        env->slot_keys = (char**)realloc(env->slot_keys, env->slot_capacity * sizeof(char*));
        env->slot_values = (Tether_ASTNode**)realloc(env->slot_values, env->slot_capacity * sizeof(Tether_ASTNode*));
    }
    env->slot_keys[env->slot_count] = strdup(key);
    env->slot_values[env->slot_count] = node;
    env->slot_count++;
}

Tether_ASTNode* tether_ast_env_get_slot(Tether_ASTEnvironment* env, const char* key) {
    if (!env || !key) return NULL;
    for (uint32_t i = 0; i < env->slot_count; i++) {
        if (strcmp(env->slot_keys[i], key) == 0) {
            return env->slot_values[i];
        }
    }
    return NULL;
}

void tether_ast_env_free(Tether_ASTEnvironment* env) {
    if (!env) return;
    for (uint32_t i = 0; i < env->count; i++) {
        free(env->keys[i]);
        free(env->values[i]);
    }
    if (env->keys) free(env->keys);
    if (env->values) free(env->values);
    
    for (uint32_t i = 0; i < env->slot_count; i++) {
        free(env->slot_keys[i]);
        /* We do not free the slot_values nodes here, they belong to the AST tree */
    }
    if (env->slot_keys) free(env->slot_keys);
    if (env->slot_values) free(env->slot_values);
    
    free(env);
}
