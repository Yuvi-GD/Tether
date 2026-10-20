#include "tether/core/tether_components.h"
#include "tether/core/tether_registry.h"

#include "tether/engine/parser/tether_yaml.h"
#include "tether/engine/parser/tether_yaml_ast.h"

#include "tether/ui/tether_widgets.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    char** names;
    Tether_ASTNode** trees;
    uint32_t count;
    uint32_t capacity;
} WidgetASTMap;

static WidgetASTMap g_widget_asts = {0};

static const char* resolve_scalar(Tether_ASTNode* node, Tether_ASTEnvironment* env) {
    if (!node) return NULL;
    if (node->type == TETHER_AST_SCALAR && node->scalar_value[0] == '$') {
        const char* var_name = node->scalar_value + 1;
        const char* val = tether_ast_env_get(env, var_name);
        if (val) return val;
    }
    return node->scalar_value;
}

static Tether_ASTNode* resolve_node(Tether_ASTNode* node, Tether_ASTEnvironment* env) {
    if (node && node->type == TETHER_AST_SCALAR && node->scalar_value[0] == '$') {
        const char* var_name = node->scalar_value + 1;
        Tether_ASTNode* slot = tether_ast_env_get_slot(env, var_name);
        if (slot) return slot;
    }
    return node;
}

static int resolve_vec2(Tether_ASTNode* node, Tether_ASTEnvironment* env, float* x, float* y) {
    if (!node || !x || !y) return 0;
    if (node->type == TETHER_AST_SCALAR) {
        float val = (float)atof(resolve_scalar(node, env));
        *x = val;
        *y = val;
        return 1;
    } else if (node->type == TETHER_AST_SEQUENCE && node->child_count > 0) {
        *x = (float)atof(resolve_scalar(node->children[0], env));
        *y = (node->child_count > 1) ? (float)atof(resolve_scalar(node->children[1], env)) : *x;
        return 1;
    }
    return 0;
}

static int resolve_vec3(Tether_ASTNode* node, Tether_ASTEnvironment* env, float* x, float* y, float* z) {
    if (!node || !x || !y || !z) return 0;
    if (node->type == TETHER_AST_SCALAR) {
        float val = (float)atof(resolve_scalar(node, env));
        *x = val; *y = val; *z = val;
        return 1;
    } else if (node->type == TETHER_AST_SEQUENCE && node->child_count > 0) {
        *x = (float)atof(resolve_scalar(node->children[0], env));
        *y = (node->child_count > 1) ? (float)atof(resolve_scalar(node->children[1], env)) : 0.0f;
        *z = (node->child_count > 2) ? (float)atof(resolve_scalar(node->children[2], env)) : 0.0f;
        return 1;
    }
    return 0;
}

static int resolve_vec4(Tether_ASTNode* node, Tether_ASTEnvironment* env, float* arr) {
    if (!node || !arr) return 0;
    if (node->type == TETHER_AST_SCALAR) {
        float val = (float)atof(resolve_scalar(node, env));
        arr[0] = val;
        arr[1] = val;
        arr[2] = val;
        arr[3] = val;
        return 1;
    } else if (node->type == TETHER_AST_SEQUENCE && node->child_count > 0) {
        if (node->child_count == 1) {
            float val = (float)atof(resolve_scalar(node->children[0], env));
            arr[0] = val; arr[1] = val; arr[2] = val; arr[3] = val;
        } else if (node->child_count == 2) {
            float v = (float)atof(resolve_scalar(node->children[0], env));
            float h = (float)atof(resolve_scalar(node->children[1], env));
            arr[0] = v; arr[1] = h; arr[2] = v; arr[3] = h;
        } else if (node->child_count == 3) {
            arr[0] = (float)atof(resolve_scalar(node->children[0], env));
            arr[1] = (float)atof(resolve_scalar(node->children[1], env));
            arr[2] = (float)atof(resolve_scalar(node->children[2], env));
            arr[3] = 0.0f;
        } else {
            arr[0] = (float)atof(resolve_scalar(node->children[0], env));
            arr[1] = (float)atof(resolve_scalar(node->children[1], env));
            arr[2] = (float)atof(resolve_scalar(node->children[2], env));
            arr[3] = (float)atof(resolve_scalar(node->children[3], env));
        }
        return 1;
    }
    return 0;
}

static int resolve_color(Tether_ASTNode* node, Tether_ASTEnvironment* env, Tether_Color* color) {
    if (!node || !color) return 0;
    if (node->type == TETHER_AST_SCALAR) {
        const char* str = resolve_scalar(node, env);
        if (str && str[0] == '#') {
            uint32_t val = (uint32_t)strtoul(str + 1, NULL, 16);
            size_t len = strlen(str + 1);
            if (len == 6) {
                color->r = (uint8_t)((val >> 16) & 0xFF);
                color->g = (uint8_t)((val >> 8) & 0xFF);
                color->b = (uint8_t)(val & 0xFF);
                color->a = 255;
                return 1;
            } else if (len == 8) {
                color->r = (uint8_t)((val >> 24) & 0xFF);
                color->g = (uint8_t)((val >> 16) & 0xFF);
                color->b = (uint8_t)((val >> 8) & 0xFF);
                color->a = (uint8_t)(val & 0xFF);
                return 1;
            }
        }
    } else if (node->type == TETHER_AST_SEQUENCE && node->child_count > 0) {
        if (node->child_count == 1) {
            uint8_t val = (uint8_t)atoi(resolve_scalar(node->children[0], env));
            color->r = val; color->g = val; color->b = val; color->a = 255;
        } else if (node->child_count == 3) {
            color->r = (uint8_t)atoi(resolve_scalar(node->children[0], env));
            color->g = (uint8_t)atoi(resolve_scalar(node->children[1], env));
            color->b = (uint8_t)atoi(resolve_scalar(node->children[2], env));
            color->a = 255;
        } else if (node->child_count >= 4) {
            color->r = (uint8_t)atoi(resolve_scalar(node->children[0], env));
            color->g = (uint8_t)atoi(resolve_scalar(node->children[1], env));
            color->b = (uint8_t)atoi(resolve_scalar(node->children[2], env));
            color->a = (uint8_t)atoi(resolve_scalar(node->children[3], env));
        }
        return 1;
    }
    return 0;
}

static int resolve_align_2d(Tether_ASTNode* node, Tether_ASTEnvironment* env, Tether_Align* align_x, Tether_Align* align_y) {
    if (!node || !align_x || !align_y) return 0;
    if (node->type == TETHER_AST_SCALAR) {
        Tether_Align a = parse_align_value(resolve_scalar(node, env));
        *align_x = a;
        *align_y = a;
        return 1;
    } else if (node->type == TETHER_AST_SEQUENCE && node->child_count > 0) {
        *align_x = parse_align_value(resolve_scalar(node->children[0], env));
        *align_y = (node->child_count > 1) ? parse_align_value(resolve_scalar(node->children[1], env)) : *align_x;
        return 1;
    }
    return 0;
}

static Tether_Align parse_align_value(const char* val) {
    if (!val) return TETHER_ALIGN_START;
    if (strcmp(val, "FILL") == 0 || strcmp(val, "fill") == 0) return TETHER_ALIGN_FILL;
    if (strcmp(val, "CENTER") == 0 || strcmp(val, "center") == 0) return TETHER_ALIGN_CENTER;
    if (strcmp(val, "END") == 0 || strcmp(val, "RIGHT") == 0 || strcmp(val, "BOTTOM") == 0 ||
        strcmp(val, "end") == 0 || strcmp(val, "right") == 0 || strcmp(val, "bottom") == 0) return TETHER_ALIGN_END;
    if (strcmp(val, "AUTO") == 0 || strcmp(val, "auto") == 0) return TETHER_ALIGN_AUTO;
    return TETHER_ALIGN_START;
}

static void register_widget_ast(Tether_ASTNode* props) {
    if (!props || props->type != TETHER_AST_MAPPING) return;
    const char* name = NULL;
    Tether_ASTNode* tree = NULL;
    
    for (uint32_t i = 0; i < props->child_count; i++) {
        Tether_ASTNode* key_node = props->keys[i];
        if (key_node && key_node->type == TETHER_AST_SCALAR) {
            if (strcmp(key_node->scalar_value, "name") == 0 && props->children[i]->type == TETHER_AST_SCALAR) {
                name = props->children[i]->scalar_value;
            } else if (strcmp(key_node->scalar_value, "tree") == 0) {
                tree = props->children[i];
            }
        }
    }
    
    if (name && tree) {
        for (uint32_t i = 0; i < g_widget_asts.count; i++) {
            if (strcmp(g_widget_asts.names[i], name) == 0) {
                g_widget_asts.trees[i] = tree;
                return;
            }
        }
        
        if (g_widget_asts.count >= g_widget_asts.capacity) {
            g_widget_asts.capacity = g_widget_asts.capacity == 0 ? 16 : g_widget_asts.capacity * 2;
            g_widget_asts.names = (char**)realloc(g_widget_asts.names, g_widget_asts.capacity * sizeof(char*));
            g_widget_asts.trees = (Tether_ASTNode**)realloc(g_widget_asts.trees, g_widget_asts.capacity * sizeof(Tether_ASTNode*));
        }
        g_widget_asts.names[g_widget_asts.count] = strdup(name);
        g_widget_asts.trees[g_widget_asts.count] = tree;
        g_widget_asts.count++;
    }
}

static Tether_ASTNode* get_widget_ast(const char* name) {
    for (uint32_t i = 0; i < g_widget_asts.count; i++) {
        if (strcmp(g_widget_asts.names[i], name) == 0) {
            return g_widget_asts.trees[i];
        }
    }
    return NULL;
}

static Tether_GUID instantiate_node(Tether_ASTNode* node, Tether_GUID parent, Tether_ASTEnvironment* env);
static void apply_properties(Tether_GUID ent, Tether_ASTNode* props, Tether_ASTEnvironment* env);

static void apply_content(Tether_GUID parent, Tether_ASTNode* content, Tether_ASTEnvironment* env) {
    if (!content || content->type != TETHER_AST_SEQUENCE) return;

    Tether_Hierarchy* hierarchy = (Tether_Hierarchy*)tether_ecs_get_component(parent, TETHER_COMPONENT_HIERARCHY);
    Tether_GUID existing_child = hierarchy ? hierarchy->first_child : TETHER_INVALID_GUID;

    for (uint32_t i = 0; i < content->child_count && existing_child != TETHER_INVALID_GUID; i++) {
        Tether_ASTNode* child_definition = resolve_node(content->children[i], env);
        if (child_definition && child_definition->type == TETHER_AST_MAPPING && child_definition->child_count > 0) {
            Tether_ASTNode* props = resolve_node(child_definition->children[0], env);
            apply_properties(existing_child, props, env);
        }

        Tether_Hierarchy* child_hierarchy = (Tether_Hierarchy*)tether_ecs_get_component(existing_child, TETHER_COMPONENT_HIERARCHY);
        existing_child = child_hierarchy ? child_hierarchy->next_sibling : TETHER_INVALID_GUID;
    }
}

static void apply_properties(Tether_GUID ent, Tether_ASTNode* props, Tether_ASTEnvironment* env) {
    if (!props || props->type != TETHER_AST_MAPPING) return;

    for (uint32_t i = 0; i < props->child_count; i++) {
        Tether_ASTNode* key_node = props->keys[i];
        Tether_ASTNode* val_node = resolve_node(props->children[i], env);
        if (!key_node || key_node->type != TETHER_AST_SCALAR || !val_node) continue;

        const char* key = key_node->scalar_value;

        if (strcmp(key, "children") == 0) {
            if (!tether_ecs_get_component(ent, TETHER_COMPONENT_IS_LEAF)) {
                instantiate_node(val_node, ent, env);
            }
        }
        else if (strcmp(key, "content") == 0) {
            if (!tether_ecs_get_component(ent, TETHER_COMPONENT_IS_LEAF)) {
                apply_content(ent, val_node, env);
            }
        }
        else if (strcmp(key, "color") == 0) {
            Tether_Shape* s = (Tether_Shape*)tether_ecs_get_component(ent, TETHER_COMPONENT_SHAPE);
            Tether_Text* t = (Tether_Text*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT);
            Tether_Color c = {0, 0, 0, 255};
            if (resolve_color(val_node, env, &c)) {
                if (s) s->bg_color = c;
                if (t) t->color = c;
            }
        }
        else if (strcmp(key, "border_color") == 0) {
            Tether_Shape* s = (Tether_Shape*)tether_ecs_get_component(ent, TETHER_COMPONENT_SHAPE);
            if (s) {
                Tether_Color c = {0, 0, 0, 255};
                if (resolve_color(val_node, env, &c)) {
                    s->border_color = c;
                }
            }
        }
        else if (strcmp(key, "border_width") == 0) {
            Tether_Shape* s = (Tether_Shape*)tether_ecs_get_component(ent, TETHER_COMPONENT_SHAPE);
            if (s && val_node->type == TETHER_AST_SCALAR) {
                s->border_width = atof(resolve_scalar(val_node, env));
            }
        }
        else if (strcmp(key, "border_radius") == 0) {
            Tether_Shape* s = (Tether_Shape*)tether_ecs_get_component(ent, TETHER_COMPONENT_SHAPE);
            if (s) {
                resolve_vec4(val_node, env, (float*)&s->border_radius);
            }
        }
        else if (strcmp(key, "overflow") == 0) {
            Tether_Overflow* of = (Tether_Overflow*)tether_ecs_get_component(ent, TETHER_COMPONENT_OVERFLOW);
            if (of && val_node->type == TETHER_AST_SCALAR) {
                const char* val = resolve_scalar(val_node, env);
                Tether_OverflowMode mode = TETHER_OVERFLOW_CLIP;
                if (strcmp(val, "VISIBLE") == 0) mode = TETHER_OVERFLOW_VISIBLE;
                else if (strcmp(val, "WRAP") == 0) mode = TETHER_OVERFLOW_WRAP;
                else if (strcmp(val, "SCROLL") == 0) mode = TETHER_OVERFLOW_SCROLL;
                of->x = mode;
                of->y = mode;
            }
        }
        else if (strcmp(key, "hover_color") == 0) {
            Tether_Interactable* i = (Tether_Interactable*)tether_ecs_get_component(ent, TETHER_COMPONENT_INTERACTABLE);
            if (i) {
                const char* val = resolve_scalar(val_node, env);
                if (val && strcmp(val, "AUTO") == 0) {
                    i->hover_color_mode = TETHER_COLOR_MODE_AUTO;
                } else if (resolve_color(val_node, env, &i->hover_color)) {
                    i->hover_color_mode = TETHER_COLOR_MODE_MANUAL;
                }
            }
        }
        else if (strcmp(key, "press_color") == 0) {
            Tether_Interactable* i = (Tether_Interactable*)tether_ecs_get_component(ent, TETHER_COMPONENT_INTERACTABLE);
            if (i) {
                const char* val = resolve_scalar(val_node, env);
                if (val && strcmp(val, "AUTO") == 0) {
                    i->press_color_mode = TETHER_COLOR_MODE_AUTO;
                } else if (resolve_color(val_node, env, &i->press_color)) {
                    i->press_color_mode = TETHER_COLOR_MODE_MANUAL;
                }
            }
        }
        else if (strcmp(key, "flow") == 0) {
            Tether_Layout* n = (Tether_Layout*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT);
            const char* val = resolve_scalar(val_node, env);
            if (n && val) {
                if (strcmp(val, "ROW") == 0) n->flow = TETHER_FLOW_ROW;
                else if (strcmp(val, "COLUMN") == 0) n->flow = TETHER_FLOW_COLUMN;
                else n->flow = TETHER_FLOW_NONE;
            }
        }
        else if (strcmp(key, "content_align") == 0) {
            Tether_Layout* n = (Tether_Layout*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT);
            if (n) resolve_align_2d(val_node, env, &n->content_align_x, &n->content_align_y);
        }
        else if (strcmp(key, "align_self") == 0) {
            Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_FLEX_SLOT);
            if (f) resolve_align_2d(val_node, env, &f->align_self_x, &f->align_self_y);
        }
        else if (strcmp(key, "text_align") == 0) {
            Tether_Text* t = (Tether_Text*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT);
            if (t) resolve_align_2d(val_node, env, &t->align_x, &t->align_y);
        }
        else if (strcmp(key, "fill_ratio") == 0) {
            Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_FLEX_SLOT);
            if (f) f->fill_ratio = (float)atof(resolve_scalar(val_node, env));
        }
        else if (strcmp(key, "anchor_min") == 0) {
            Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
            if (a) resolve_vec2(val_node, env, &a->anchor_min.x, &a->anchor_min.y);
        }
        else if (strcmp(key, "anchor_max") == 0) {
            Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
            if (a) resolve_vec2(val_node, env, &a->anchor_max.x, &a->anchor_max.y);
        }
        else if (strcmp(key, "offset") == 0) {
            Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
            if (a) resolve_vec4(val_node, env, (float*)&a->offset);
        }
        else if (strcmp(key, "pivot") == 0) {
            Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
            if (a) resolve_vec2(val_node, env, &a->pivot.x, &a->pivot.y);
        }
        else if (strcmp(key, "margin") == 0) {
            Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_FLEX_SLOT);
            if (f) resolve_vec4(val_node, env, (float*)&f->margin);
        }
        else if (strcmp(key, "padding") == 0) {
            Tether_Layout* n = (Tether_Layout*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT);
            if (n) resolve_vec4(val_node, env, (float*)&n->padding);
        }
        else if (strcmp(key, "gap") == 0) {
            Tether_Layout* n = (Tether_Layout*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT);
            if (n) resolve_vec2(val_node, env, &n->gap.x, &n->gap.y);
        }
        else if (strcmp(key, "wrap") == 0) {
            Tether_Layout* n = (Tether_Layout*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT);
            if (n) {
                const char* val = resolve_scalar(val_node, env);
                if (val && (strcmp(val, "true") == 0 || strcmp(val, "1") == 0)) n->wrap = 1;
                else n->wrap = 0;
            }
        }
        else if (strcmp(key, "size_box") == 0) {
            Tether_Layout* n = (Tether_Layout*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT);
            if (n) resolve_vec2(val_node, env, &n->size_box.x, &n->size_box.y);
        }
        else if (strcmp(key, "id") == 0) {
            const char* val = resolve_scalar(val_node, env);
            if (val) {
                tether_registry_alias_entity(ent, val);
            }
        }
        else if (strcmp(key, "hit_behavior") == 0) {
            Tether_Interactable* i = (Tether_Interactable*)tether_ecs_get_component(ent, TETHER_COMPONENT_INTERACTABLE);
            const char* val = resolve_scalar(val_node, env);
            if (i && val) {
                if (strcmp(val, "BLOCK") == 0) i->hit_behavior = TETHER_HIT_BLOCK;
                else if (strcmp(val, "IGNORE") == 0) i->hit_behavior = TETHER_HIT_IGNORE;
                else if (strcmp(val, "SELF_ONLY") == 0) i->hit_behavior = TETHER_HIT_SELF_ONLY;
                else if (strcmp(val, "CHILD_ONLY") == 0) i->hit_behavior = TETHER_HIT_CHILD_ONLY;
            }
        }
        else if (strcmp(key, "visibility") == 0) {
            Tether_Visibility* vis = (Tether_Visibility*)tether_ecs_get_component(ent, TETHER_COMPONENT_VISIBILITY);
            const char* val = resolve_scalar(val_node, env);
            if (vis && val) {
                if (strcmp(val, "hidden") == 0) tether_widget_set_visibility(ent, TETHER_HIDDEN);
                else if (strcmp(val, "collapsed") == 0) tether_widget_set_visibility(ent, TETHER_COLLAPSED);
                else tether_widget_set_visibility(ent, TETHER_VISIBLE);
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
                tether_ecs_set_text_string(ent, val);
            }
        }
        else if (strcmp(key, "font_size") == 0) {
            Tether_Text* t = (Tether_Text*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT);
            if (t) t->font_size = (float)atof(resolve_scalar(val_node, env));
        }
        else if (strcmp(key, "wrap_width") == 0) {
            Tether_Text* t = (Tether_Text*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT);
            if (t) t->wrap_width = (float)atof(resolve_scalar(val_node, env));
        }
        else if (strcmp(key, "dynamic") == 0) {
            const char* val = resolve_scalar(val_node, env);
            if (val && (strcmp(val, "true") == 0 || strcmp(val, "1") == 0)) {
                tether_ecs_add_component(ent, TETHER_COMPONENT_TEXT_DYNAMIC);
            }
        }
        else if (strcmp(key, "opacity") == 0) {
            Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_get_component(ent, TETHER_COMPONENT_RENDER_TRANSFORM);
            if (rt) rt->opacity = (float)atof(resolve_scalar(val_node, env));
        }
        else if (strcmp(key, "rotation") == 0) {
            Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_get_component(ent, TETHER_COMPONENT_RENDER_TRANSFORM);
            if (rt) rt->rotation_deg = (float)atof(resolve_scalar(val_node, env));
        }
        else if (strcmp(key, "scale") == 0) {
            Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_get_component(ent, TETHER_COMPONENT_RENDER_TRANSFORM);
            if (rt) resolve_vec2(val_node, env, &rt->scale_x, &rt->scale_y);
        }
        else if (strcmp(key, "translation") == 0) {
            Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_get_component(ent, TETHER_COMPONENT_RENDER_TRANSFORM);
            if (rt) resolve_vec2(val_node, env, &rt->translation_x, &rt->translation_y);
        }
        else if (strcmp(key, "render_pivot") == 0) {
            Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_get_component(ent, TETHER_COMPONENT_RENDER_TRANSFORM);
            if (rt) resolve_vec2(val_node, env, &rt->render_pivot.x, &rt->render_pivot.y);
        }
    }
}

static Tether_GUID instantiate_node(Tether_ASTNode* node, Tether_GUID parent, Tether_ASTEnvironment* env) {
    if (!node) return TETHER_INVALID_GUID;

    if (node->type == TETHER_AST_SEQUENCE) {
        Tether_GUID first_child = TETHER_INVALID_GUID;
        for (uint32_t i = 0; i < node->child_count; i++) {
            Tether_GUID child = instantiate_node(node->children[i], parent, env);
            if (first_child == TETHER_INVALID_GUID) first_child = child;
        }
        return first_child;
    } 
    else if (node->type == TETHER_AST_MAPPING) {
        /* A widget definition is a mapping where one of the keys matches a registered widget. */
        for (uint32_t i = 0; i < node->child_count; i++) {
            Tether_ASTNode* key_node = node->keys[i];
            if (key_node && key_node->type == TETHER_AST_SCALAR) {
                const char* key_str = key_node->scalar_value;
                
                if (strcmp(key_str, "Widget") == 0) {
                    continue; /* Handled in pre-pass */
                }

                if (strcmp(key_str, "Scene") == 0) {
                    const char* scene_path = resolve_scalar(node->children[i], env);
                    if (scene_path) return tether_yaml_load(scene_path, parent);
                    return TETHER_INVALID_GUID;
                }
                
                if (strcmp(key_str, "Slot") == 0) {
                    Tether_ASTNode* slot_name_node = node->children[i];
                    if (slot_name_node && slot_name_node->type == TETHER_AST_SCALAR) {
                        Tether_ASTNode* injected = tether_ast_env_get_slot(env, slot_name_node->scalar_value);
                        if (injected) {
                            return instantiate_node(injected, parent, env);
                        }
                    }
                    continue;
                }
                
                /* Check if it's a custom widget */
                Tether_ASTNode* widget_tree = get_widget_ast(key_str);
                if (widget_tree) {
                    Tether_ASTNode* props = node->children[i];
                    Tether_ASTEnvironment* new_env = tether_ast_env_create();
                    
                    if (props && props->type == TETHER_AST_MAPPING) {
                        for (uint32_t p = 0; p < props->child_count; p++) {
                            Tether_ASTNode* p_key = props->keys[p];
                            Tether_ASTNode* p_val = props->children[p];
                            if (p_key && p_key->type == TETHER_AST_SCALAR) {
                                if (p_val && p_val->type != TETHER_AST_SCALAR) {
                                    tether_ast_env_set_slot(new_env, p_key->scalar_value, p_val);
                                } else if (p_val) {
                                    tether_ast_env_set(new_env, p_key->scalar_value, resolve_scalar(p_val, env));
                                }
                            }
                        }
                    } else {
                        /* Flat syntax fallback */
                        for (uint32_t p = 0; p < node->child_count; p++) {
                            if (p == i) continue; /* Skip the Widget name key itself */
                            Tether_ASTNode* p_key = node->keys[p];
                            Tether_ASTNode* p_val = node->children[p];
                            if (p_key && p_key->type == TETHER_AST_SCALAR) {
                                if (p_val && p_val->type != TETHER_AST_SCALAR) {
                                    tether_ast_env_set_slot(new_env, p_key->scalar_value, p_val);
                                } else if (p_val) {
                                    tether_ast_env_set(new_env, p_key->scalar_value, resolve_scalar(p_val, env));
                                }
                            }
                        }
                    }
                    
                    Tether_GUID new_ent = instantiate_node(widget_tree, parent, new_env);
                    tether_ast_env_free(new_env);
                    return new_ent;
                }
                
                /* Check native widgets */
                Tether_GUID new_ent = tether_create_widget(key_str, parent);
                if (new_ent != TETHER_INVALID_GUID) {
                    Tether_ASTNode* props = node->children[i];
                    if (props && props->type == TETHER_AST_MAPPING) {
                        apply_properties(new_ent, props, env);
                    } else {
                        /* Flat syntax fallback */
                        apply_properties(new_ent, node, env);
                    }
                    return new_ent;
                }
            }
        }
    }

    return TETHER_INVALID_GUID;
}

Tether_GUID tether_yaml_load(const char* filepath, Tether_GUID parent) {
    Tether_ASTNode* ast = tether_ast_parse_file(filepath);
    if (!ast) return TETHER_INVALID_GUID;

    /* Pre-pass: Find and cache widgets */
    if (ast->type == TETHER_AST_SEQUENCE) {
        for (uint32_t i = 0; i < ast->child_count; i++) {
            Tether_ASTNode* child = ast->children[i];
            if (child->type == TETHER_AST_MAPPING) {
                for (uint32_t j = 0; j < child->child_count; j++) {
                    Tether_ASTNode* key_node = child->keys[j];
                    if (key_node && key_node->type == TETHER_AST_SCALAR && strcmp(key_node->scalar_value, "Widget") == 0) {
                        register_widget_ast(child->children[j]);
                    }
                }
            }
        }
    }

    if (parent == TETHER_INVALID_GUID) {
        parent = tether_ecs_get_main_root();
    }
    
    /* Use a blank environment for the root document */
    Tether_ASTEnvironment* env = tether_ast_env_create();
    
    /* Instantiate */
    Tether_GUID root = instantiate_node(ast, parent, env);
    
    if (root != TETHER_INVALID_GUID) {
        tether_widget_show(root);
    }
    
    tether_ast_env_free(env);
    tether_ast_free(ast);
    return root;
}
