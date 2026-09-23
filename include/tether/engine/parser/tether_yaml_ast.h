#ifndef TETHER_YAML_AST_H
#define TETHER_YAML_AST_H

#include "tether/core/tether_ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TETHER_AST_SCALAR,
    TETHER_AST_SEQUENCE,
    TETHER_AST_MAPPING
} Tether_ASTNodeType;

/* Forward declaration */
struct Tether_ASTNode;
typedef struct Tether_ASTNode Tether_ASTNode;

struct Tether_ASTNode {
    Tether_ASTNodeType type;
    
    /* SCALAR: The string value */
    char* scalar_value;
    
    /* SEQUENCE / MAPPING: Array of child nodes */
    Tether_ASTNode** children;
    uint32_t child_count;
    uint32_t child_capacity;
    
    /* MAPPING ONLY: Array of key nodes (same size as children) */
    Tether_ASTNode** keys;
};

/* 
 * Environment map for parameter injection (e.g. $label -> "Click Me").
 * A simple dictionary of string -> string.
 */
typedef struct {
    /* String variables */
    char** keys;
    char** values;
    uint32_t count;
    uint32_t capacity;

    /* AST Node slots */
    char** slot_keys;
    Tether_ASTNode** slot_values;
    uint32_t slot_count;
    uint32_t slot_capacity;
} Tether_ASTEnvironment;

/* Parse a YAML file into an AST tree. Returns NULL on failure. */
Tether_ASTNode* tether_ast_parse_file(const char* filepath);

/* Free the AST tree */
void tether_ast_free(Tether_ASTNode* node);

/* Environment API */
Tether_ASTEnvironment* tether_ast_env_create(void);
void tether_ast_env_set(Tether_ASTEnvironment* env, const char* key, const char* value);
const char* tether_ast_env_get(Tether_ASTEnvironment* env, const char* key);
void tether_ast_env_set_slot(Tether_ASTEnvironment* env, const char* key, Tether_ASTNode* node);
Tether_ASTNode* tether_ast_env_get_slot(Tether_ASTEnvironment* env, const char* key);
void tether_ast_env_free(Tether_ASTEnvironment* env);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_YAML_AST_H */
