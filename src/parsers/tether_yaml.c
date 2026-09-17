#include "parsers/tether_yaml.h"
#include "parsers/tether_yaml_ast.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* resolve_scalar(Tether_ASTNode* node, Tether_ASTEnvironment* env) {
    if (!node || node->type != TETHER_AST_SCALAR || !node->scalar_value) return NULL;
    if (node->scalar_value[0] == '$' && env) {
        const char* val = tether_ast_env_get(env, node->scalar_value + 1);
        if (val) return val;
    }
    return node->scalar_value;
}

static Tether_GUID instantiate_node(Tether_ASTNode* node, Tether_GUID parent, Tether_ASTEnvironment* env);

static void apply_properties(Tether_GUID ent, Tether_ASTNode* props, Tether_ASTEnvironment* env) {
    if (!props || props->type != TETHER_AST_MAPPING) return;
    
    for (uint32_t i = 0; i < props->child_count; i++) {
        Tether_ASTNode* key_node = props->keys[i];
        Tether_ASTNode* val_node = props->children[i];
        if (!key_node || key_node->type != TETHER_AST_SCALAR) continue;
        const char* key = key_node->scalar_value;
        
        if (strcmp(key, "children") == 0) {
            instantiate_node(val_node, ent, env);
        }
        else if (strcmp(key, "color") == 0) {
            Tether_Style* s = (Tether_Style*)tether_ecs_get_component(ent, TETHER_COMPONENT_STYLE);
            if (!s) s = (Tether_Style*)tether_ecs_add_component(ent, TETHER_COMPONENT_STYLE);
            if (val_node->type == TETHER_AST_SEQUENCE) {
                if (val_node->child_count > 0) s->bg_color.r = atoi(resolve_scalar(val_node->children[0], env));
                if (val_node->child_count > 1) s->bg_color.g = atoi(resolve_scalar(val_node->children[1], env));
                if (val_node->child_count > 2) s->bg_color.b = atoi(resolve_scalar(val_node->children[2], env));
                if (val_node->child_count > 3) s->bg_color.a = atoi(resolve_scalar(val_node->children[3], env));
            }
        }
        else if (strcmp(key, "hover_color") == 0) {
            Tether_Style* s = (Tether_Style*)tether_ecs_get_component(ent, TETHER_COMPONENT_STYLE);
            if (!s) s = (Tether_Style*)tether_ecs_add_component(ent, TETHER_COMPONENT_STYLE);
            if (val_node->type == TETHER_AST_SCALAR && strcmp(resolve_scalar(val_node, env), "AUTO") == 0) {
                s->hover_color_mode = TETHER_COLOR_MODE_AUTO;
            } else if (val_node->type == TETHER_AST_SEQUENCE) {
                s->hover_color_mode = TETHER_COLOR_MODE_MANUAL;
                if (val_node->child_count > 0) s->hover_color.r = atoi(resolve_scalar(val_node->children[0], env));
                if (val_node->child_count > 1) s->hover_color.g = atoi(resolve_scalar(val_node->children[1], env));
                if (val_node->child_count > 2) s->hover_color.b = atoi(resolve_scalar(val_node->children[2], env));
                if (val_node->child_count > 3) s->hover_color.a = atoi(resolve_scalar(val_node->children[3], env));
            }
        }
        else if (strcmp(key, "press_color") == 0) {
            Tether_Style* s = (Tether_Style*)tether_ecs_get_component(ent, TETHER_COMPONENT_STYLE);
            if (!s) s = (Tether_Style*)tether_ecs_add_component(ent, TETHER_COMPONENT_STYLE);
            if (val_node->type == TETHER_AST_SCALAR && strcmp(resolve_scalar(val_node, env), "AUTO") == 0) {
                s->press_color_mode = TETHER_COLOR_MODE_AUTO;
            } else if (val_node->type == TETHER_AST_SEQUENCE) {
                s->press_color_mode = TETHER_COLOR_MODE_MANUAL;
                if (val_node->child_count > 0) s->press_color.r = atoi(resolve_scalar(val_node->children[0], env));
                if (val_node->child_count > 1) s->press_color.g = atoi(resolve_scalar(val_node->children[1], env));
                if (val_node->child_count > 2) s->press_color.b = atoi(resolve_scalar(val_node->children[2], env));
                if (val_node->child_count > 3) s->press_color.a = atoi(resolve_scalar(val_node->children[3], env));
            }
        }
        else if (strcmp(key, "flow") == 0) {
            Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
            const char* val = resolve_scalar(val_node, env);
            if (val) {
                if (strcmp(val, "ROW") == 0) n->flow = TETHER_FLOW_ROW;
                else if (strcmp(val, "COLUMN") == 0) n->flow = TETHER_FLOW_COLUMN;
                else n->flow = TETHER_FLOW_NONE;
            }
        }
        else if (strcmp(key, "fill_ratio") == 0) {
            Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_FLEX_SLOT);
            if (f) f->fill_ratio = (float)atof(resolve_scalar(val_node, env));
        }
        else if (strcmp(key, "anchor_min") == 0) {
            Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
            if (a && val_node->type == TETHER_AST_SEQUENCE) {
                if (val_node->child_count > 0) a->anchor_min.x = (float)atof(resolve_scalar(val_node->children[0], env));
                if (val_node->child_count > 1) a->anchor_min.y = (float)atof(resolve_scalar(val_node->children[1], env));
            }
        }
        else if (strcmp(key, "anchor_max") == 0) {
            Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
            if (a && val_node->type == TETHER_AST_SEQUENCE) {
                if (val_node->child_count > 0) a->anchor_max.x = (float)atof(resolve_scalar(val_node->children[0], env));
                if (val_node->child_count > 1) a->anchor_max.y = (float)atof(resolve_scalar(val_node->children[1], env));
            }
        }
        else if (strcmp(key, "offset") == 0) {
            Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
            if (a && val_node->type == TETHER_AST_SEQUENCE) {
                float* arr = (float*)&a->offset;
                for (uint32_t j = 0; j < val_node->child_count && j < 4; j++) {
                    arr[j] = (float)atof(resolve_scalar(val_node->children[j], env));
                }
            }
        }
        else if (strcmp(key, "margin") == 0) {
            Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_FLEX_SLOT);
            if (f && val_node->type == TETHER_AST_SEQUENCE) {
                float* arr = (float*)&f->margin;
                for (uint32_t j = 0; j < val_node->child_count && j < 4; j++) {
                    arr[j] = (float)atof(resolve_scalar(val_node->children[j], env));
                }
            }
        }
        else if (strcmp(key, "padding") == 0) {
            Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
            if (n && val_node->type == TETHER_AST_SEQUENCE) {
                float* arr = (float*)&n->padding;
                for (uint32_t j = 0; j < val_node->child_count && j < 4; j++) {
                    arr[j] = (float)atof(resolve_scalar(val_node->children[j], env));
                }
            }
        }
        else if (strcmp(key, "gap") == 0) {
            Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
            if (n && val_node->type == TETHER_AST_SEQUENCE) {
                if (val_node->child_count > 0) n->gap.x = (float)atof(resolve_scalar(val_node->children[0], env));
                if (val_node->child_count > 1) n->gap.y = (float)atof(resolve_scalar(val_node->children[1], env));
            }
        }
        else if (strcmp(key, "wrap") == 0) {
            Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
            if (n) {
                const char* val = resolve_scalar(val_node, env);
                if (val && (strcmp(val, "true") == 0 || strcmp(val, "1") == 0)) n->wrap = 1;
                else n->wrap = 0;
            }
        }
        else if (strcmp(key, "explicit_size") == 0) {
            Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_FLEX_SLOT);
            if (f && val_node->type == TETHER_AST_SEQUENCE) {
                if (val_node->child_count > 0) f->explicit_size.x = (float)atof(resolve_scalar(val_node->children[0], env));
                if (val_node->child_count > 1) f->explicit_size.y = (float)atof(resolve_scalar(val_node->children[1], env));
            }
        }
        else if (strcmp(key, "id") == 0) {
            Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
            const char* val = resolve_scalar(val_node, env);
            if (n && val) {
                strncpy(n->id, val, 31);
                n->id[31] = '\0';
            }
        }
        else if (strcmp(key, "hit_behavior") == 0) {
            Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
            const char* val = resolve_scalar(val_node, env);
            if (n && val) {
                if (strcmp(val, "BLOCK") == 0) n->hit_behavior = TETHER_HIT_BLOCK;
                else if (strcmp(val, "IGNORE_SELF") == 0) n->hit_behavior = TETHER_HIT_IGNORE_SELF;
                else if (strcmp(val, "IGNORE_ALL") == 0) n->hit_behavior = TETHER_HIT_IGNORE_ALL;
            }
        }
        else if (strcmp(key, "visibility") == 0) {
            Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
            const char* val = resolve_scalar(val_node, env);
            if (n && val) {
                if (strcmp(val, "hidden") == 0) n->visibility = TETHER_HIDDEN;
                else if (strcmp(val, "collapsed") == 0) n->visibility = TETHER_COLLAPSED;
                else n->visibility = TETHER_VISIBLE;
            }
        }
        else if (strcmp(key, "align_x") == 0) {
            Tether_TextStyle* t = (Tether_TextStyle*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT_STYLE);
            const char* val = resolve_scalar(val_node, env);
            if (t && val) {
                if (strcmp(val, "fill") == 0) t->align_x = TETHER_ALIGN_FILL;
                else if (strcmp(val, "center") == 0) t->align_x = TETHER_ALIGN_CENTER;
                else if (strcmp(val, "end") == 0 || strcmp(val, "right") == 0) t->align_x = TETHER_ALIGN_END;
                else t->align_x = TETHER_ALIGN_START;
            }
        }
        else if (strcmp(key, "align_y") == 0) {
            Tether_TextStyle* t = (Tether_TextStyle*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT_STYLE);
            const char* val = resolve_scalar(val_node, env);
            if (t && val) {
                if (strcmp(val, "fill") == 0) t->align_y = TETHER_ALIGN_FILL;
                else if (strcmp(val, "center") == 0) t->align_y = TETHER_ALIGN_CENTER;
                else if (strcmp(val, "end") == 0 || strcmp(val, "bottom") == 0) t->align_y = TETHER_ALIGN_END;
                else t->align_y = TETHER_ALIGN_START;
            }
        }
        else if (strcmp(key, "interactable") == 0) {
            const char* val = resolve_scalar(val_node, env);
            if (val && (strcmp(val, "true") == 0 || strcmp(val, "1") == 0)) {
                tether_ecs_add_component(ent, TETHER_COMPONENT_INTERACTABLE);
            }
        }
        else if (strcmp(key, "string") == 0) {
            const char* val = resolve_scalar(val_node, env);
            if (val) {
                size_t len = strlen(val);
                Tether_TextDynamic* dyn = (Tether_TextDynamic*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT_DYNAMIC);
                
                if (dyn || len > 511) {
                    if (!dyn) dyn = (Tether_TextDynamic*)tether_ecs_add_component(ent, TETHER_COMPONENT_TEXT_DYNAMIC);
                    dyn->capacity = (uint32_t)(len + 1) * 2;
                    dyn->data = (char*)malloc(dyn->capacity);
                    strcpy(dyn->data, val);
                    dyn->length = (uint32_t)len;
                } else if (len <= 31) {
                    Tether_TextWord* t = (Tether_TextWord*)tether_ecs_add_component(ent, TETHER_COMPONENT_TEXT_WORD);
                    strncpy(t->data, val, 31);
                    t->data[31] = '\0';
                } else if (len <= 127) {
                    Tether_TextLabel* t = (Tether_TextLabel*)tether_ecs_add_component(ent, TETHER_COMPONENT_TEXT_LABEL);
                    strncpy(t->data, val, 127);
                    t->data[127] = '\0';
                } else if (len <= 511) {
                    Tether_TextParagraph* t = (Tether_TextParagraph*)tether_ecs_add_component(ent, TETHER_COMPONENT_TEXT_PARAGRAPH);
                    strncpy(t->data, val, 511);
                    t->data[511] = '\0';
                }
            }
        }
        else if (strcmp(key, "font_size") == 0) {
            Tether_TextStyle* t = (Tether_TextStyle*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT_STYLE);
            if (t) t->font_size = (float)atof(resolve_scalar(val_node, env));
        }
        else if (strcmp(key, "wrap_width") == 0) {
            Tether_TextStyle* t = (Tether_TextStyle*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT_STYLE);
            if (t) t->wrap_width = (float)atof(resolve_scalar(val_node, env));
        }
        else if (strcmp(key, "dynamic") == 0) {
            const char* val = resolve_scalar(val_node, env);
            if (val && (strcmp(val, "true") == 0 || strcmp(val, "1") == 0)) {
                tether_ecs_add_component(ent, TETHER_COMPONENT_TEXT_DYNAMIC);
            }
        }
    }
}

static Tether_GUID instantiate_node(Tether_ASTNode* node, Tether_GUID parent, Tether_ASTEnvironment* env) {
    if (!node) return TETHER_INVALID_GUID;

    if (node->type == TETHER_AST_SEQUENCE) {
        Tether_GUID first_child = TETHER_INVALID_GUID;
        for (uint32_t i = 0; i < node->child_count; i++) {
            Tether_GUID child = instantiate_node(node->children[i], parent, env);
            if (i == 0) first_child = child;
        }
        return first_child;
    } 
    else if (node->type == TETHER_AST_MAPPING) {
        /* A widget definition is a mapping where one of the keys matches a registered widget. */
        for (uint32_t i = 0; i < node->child_count; i++) {
            Tether_ASTNode* key_node = node->keys[i];
            if (key_node && key_node->type == TETHER_AST_SCALAR) {
                const char* key_str = key_node->scalar_value;
                
                Tether_GUID new_ent = tether_create_widget(key_str, parent);
                if (new_ent != TETHER_INVALID_GUID) {
                    Tether_ASTNode* props = node->children[i];
                    if (props && props->type == TETHER_AST_MAPPING) {
                        apply_properties(new_ent, props, env);
                    }
                    return new_ent;
                }
            }
        }
    }

    return TETHER_INVALID_GUID;
}

Tether_GUID tether_yaml_load(const char* filepath) {
    Tether_ASTNode* ast = tether_ast_parse_file(filepath);
    if (!ast) return TETHER_INVALID_GUID;

    /* Use a blank environment for the root document */
    Tether_ASTEnvironment* env = tether_ast_env_create();
    
    /* Instantiate */
    Tether_GUID root = instantiate_node(ast, TETHER_INVALID_GUID, env);
    
    tether_ast_env_free(env);
    tether_ast_free(ast);
    return root;
}
