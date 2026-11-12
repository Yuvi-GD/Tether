#include "tether/engine/parser/tether_yaml_widgets.h"
#include "tether/core/tether_registry.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_list.h"
#include "tether/ui/widgets/tether_checkbox.h"
#include "tether/ui/widgets/tether_radio.h"
#include "tether/ui/widgets/tether_segmented_bar.h"
#include "tether/ui/widgets/tether_dropdown.h"
#include "tether/ui/widgets/tether_collapsible.h"
#include "tether/ui/widgets/tether_switcher.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include "tether/ui/widgets/tether_menu.h"
#include <stdlib.h>
#include <string.h>

bool tether_yaml_is_builtin_widget(const char* name) {
    if (!name) return false;
    return (strcmp(name, "ProgressBar") == 0 ||
            strcmp(name, "Slider") == 0 ||
            strcmp(name, "Toggle") == 0 ||
            strcmp(name, "Checkbox") == 0 ||
            strcmp(name, "Radio") == 0 ||
            strcmp(name, "SegmentedBar") == 0 ||
            strcmp(name, "Dropdown") == 0 ||
            strcmp(name, "Collapsible") == 0 ||
            strcmp(name, "Switcher") == 0);
}

static const char* resolve_val(Tether_ASTNode* node, Tether_ASTEnvironment* env) {
    if (!node) return NULL;
    if (node->type == TETHER_AST_SCALAR && node->scalar_value && node->scalar_value[0] == '$') {
        const char* var_name = node->scalar_value + 1;
        const char* val = tether_ast_env_get(env, var_name);
        if (val) return val;
    }
    return node->scalar_value;
}

static Tether_ASTNode* find_property(Tether_ASTNode* mapping, const char* name) {
    if (!mapping || mapping->type != TETHER_AST_MAPPING) return NULL;
    for (uint32_t i = 0; i < mapping->child_count; ++i) {
        Tether_ASTNode* key = mapping->keys[i];
        if (key && key->type == TETHER_AST_SCALAR && !strcmp(key->scalar_value, name)) return mapping->children[i];
    }
    return NULL;
}

static Tether_ASTNode* resolve_node(Tether_ASTNode* node, Tether_ASTEnvironment* env) {
    if (node && node->type == TETHER_AST_SCALAR && node->scalar_value[0] == '$') {
        Tether_ASTNode* slot = tether_ast_env_get_slot(env, node->scalar_value + 1);
        if (slot) return slot;
    }
    return node;
}

static bool read_string(Tether_ASTNode* node, Tether_ASTEnvironment* env, const char** value) {
    node = resolve_node(node, env);
    if (!node || node->type != TETHER_AST_SCALAR) return false;
    *value = resolve_val(node, env);
    return *value != NULL;
}

static bool read_bool(Tether_ASTNode* node, Tether_ASTEnvironment* env, bool* value) {
    const char* text;
    if (!value || !read_string(node, env, &text)) return false;
    if (!strcmp(text, "true") || !strcmp(text, "1")) {
        *value = true;
    } else if (!strcmp(text, "false") || !strcmp(text, "0")) {
        *value = false;
    } else {
        return false;
    }
    return true;
}

static bool read_int(Tether_ASTNode* node, Tether_ASTEnvironment* env, int* value) {
    const char* text;
    if (!value || !read_string(node, env, &text)) return false;
    char* end;
    errno = 0;
    long parsed = strtol(text, &end, 10);
    if (end == text || *end || errno == ERANGE || parsed < INT_MIN || parsed > INT_MAX) return false;
    *value = (int)parsed;
    return true;
}

static bool apply_dropdown(Tether_GUID entity, Tether_ASTNode* props, Tether_ASTEnvironment* env) {
    Tether_ASTNode* options = resolve_node(find_property(props, "options"), env);
    Tether_ASTNode* selected_node = find_property(props, "selected");
    Tether_ASTNode* open_node = find_property(props, "open");
    Tether_ASTNode* type_node = find_property(props, "type");
    int selected = 0;
    bool open = false;
    const char* type = NULL;

    if (options && (options->type != TETHER_AST_SEQUENCE || options->child_count > INT_MAX)) {
        fprintf(stderr, "YAML Dropdown: options must be a sequence\n");
        return false;
    }
    Tether_GUID menu = tether_widget_get_child_at(entity, 1);
    int count = options ? (int)options->child_count : tether_ui_menu_get_item_count(menu);
    if (selected_node && (!read_int(selected_node, env, &selected) ||
                          selected < 0 || selected >= count)) {
        fprintf(stderr, "YAML Dropdown: selected must index an existing option\n");
        return false;
    }
    if (open_node && !read_bool(open_node, env, &open)) {
        fprintf(stderr, "YAML Dropdown: open must be true/false or 1/0\n");
        return false;
    }
    if (type_node && (!read_string(type_node, env, &type) ||
                     (strcmp(type, "inline") && strcmp(type, "INLINE") &&
                      strcmp(type, "overlay") && strcmp(type, "OVERLAY")))) {
        fprintf(stderr, "YAML Dropdown: unknown type\n");
        return false;
    }

    const char** labels = NULL;
    if (options && count) {
        if ((size_t)count > SIZE_MAX / sizeof(*labels)) return false;
        labels = malloc((size_t)count * sizeof(*labels));
        if (!labels) return false;
        for (int i = 0; i < count; ++i) {
            if (!read_string(options->children[i], env, &labels[i])) {
                fprintf(stderr, "YAML Dropdown: option %d must be a scalar string\n", i);
                free(labels);
                return false;
            }
        }
    }

    /* Dependencies, not YAML field order, determine application order.
     * Native setters copy strings. Only this temporary pointer array is owned here.
     */
    if (labels) tether_ui_set_dropdown_options(entity, labels, count);
    free(labels);
    if (selected_node) tether_ui_set_dropdown_selected(entity, selected);
    if (type) {
        tether_ui_set_dropdown_type(entity, (!strcmp(type, "overlay") || !strcmp(type, "OVERLAY"))
                                           ? TETHER_DROPDOWN_OVERLAY : TETHER_DROPDOWN_INLINE);
    }
    if (open_node) tether_ui_set_dropdown_open(entity, open);
    return true;
}

bool tether_yaml_apply_widget_properties(Tether_GUID entity, const char* widget_name,
                                         Tether_ASTNode* props, Tether_ASTEnvironment* env) {
    if (!props || !widget_name || entity == TETHER_INVALID_GUID) return false;
    if (props->type != TETHER_AST_MAPPING) return false;

    if (!strcmp(widget_name, "Dropdown")) return apply_dropdown(entity, props, env);
    bool is_radio = (strcmp(widget_name, "Radio") == 0);
    bool is_seg = (strcmp(widget_name, "SegmentedBar") == 0);
    bool is_col = (strcmp(widget_name, "Collapsible") == 0);
    bool is_switcher = (strcmp(widget_name, "Switcher") == 0);

    for (uint32_t i = 0; i < props->child_count; i++) {
        Tether_ASTNode* k_node = props->keys[i];
        Tether_ASTNode* v_node = props->children[i];
        if (!k_node || k_node->type != TETHER_AST_SCALAR) continue;
        const char* key = k_node->scalar_value;
        const char* val = resolve_val(v_node, env);

        if (is_radio && strcmp(key, "options") == 0 && v_node) {
            if (v_node->type == TETHER_AST_SEQUENCE) {
                for (uint32_t j = 0; j < v_node->child_count; j++) {
                    const char* opt = resolve_val(v_node->children[j], env);
                    if (opt) tether_ui_add_radio_option(entity, opt);
                }
            }
        }
        else if (is_radio && strcmp(key, "selected") == 0 && val) {
            if (strcmp(val, "false") == 0 || strcmp(val, "-1") == 0) {
                tether_ui_unselect_radio(entity);
            } else if (strcmp(val, "true") == 0) {
                tether_ui_set_radio_selected(entity, 0);
            } else {
                tether_ui_set_radio_selected(entity, atoi(val));
            }
        }
        else if (is_radio && strcmp(key, "label") == 0 && val) {
            if (tether_list_get_count(entity) == 0) {
                tether_ui_add_radio_option(entity, val);
            } else {
                tether_ui_set_radio_option(entity, 0, val);
            }
        }
        else if (is_seg && strcmp(key, "options") == 0 && v_node) {
            if (v_node->type == TETHER_AST_SEQUENCE) {
                for (uint32_t j = 0; j < v_node->child_count; j++) {
                    const char* opt = resolve_val(v_node->children[j], env);
                    if (opt) tether_ui_add_segment(entity, opt);
                }
            }
        }
        else if (is_seg && strcmp(key, "selected") == 0 && val) {
            tether_ui_set_segment_selected(entity, atoi(val));
        }
        else if (is_col && strcmp(key, "title") == 0 && val) {
            Tether_Hierarchy *oh = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
            if (oh && oh->first_child != TETHER_INVALID_GUID) {
                Tether_Hierarchy *hh = (Tether_Hierarchy*)tether_ecs_get_component(oh->first_child, TETHER_COMPONENT_HIERARCHY);
                if (hh && hh->first_child != TETHER_INVALID_GUID) {
                    Tether_Hierarchy *ah = (Tether_Hierarchy*)tether_ecs_get_component(hh->first_child, TETHER_COMPONENT_HIERARCHY);
                    if (ah && ah->next_sibling != TETHER_INVALID_GUID) {
                        tether_ui_set_text(ah->next_sibling, val);
                    }
                }
            }
        }
        else if (is_col && strcmp(key, "open") == 0 && val) {
            bool open = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
            if (!open && tether_ui_is_collapsible_open(entity)) {
                tether_ui_toggle_collapsible(entity);
            }
        }
        else if (is_switcher && strcmp(key, "active") == 0 && val) {
            tether_ui_switcher_show_index(entity, atoi(val));
        }
    }

    /* Sync visuals after YAML properties have been applied */
    if (strcmp(widget_name, "Slider") == 0) {
        Tether_FlexSlot *f = (Tether_FlexSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_FLEX_SLOT);
        if (f) {
            f->align_self_x = TETHER_ALIGN_FILL;
            f->fill_ratio = 0.0f;
        }
        tether_ui_set_slider_value(entity, tether_ui_get_slider_value(entity));
    } else if (strcmp(widget_name, "Toggle") == 0) {
        tether_ui_set_toggle(entity, tether_ui_get_toggle(entity));
    } else if (strcmp(widget_name, "Checkbox") == 0) {
        tether_ui_set_checked(entity, tether_ui_get_checked(entity));
    } else if (strcmp(widget_name, "ProgressBar") == 0) {
        Tether_FlexSlot *f = (Tether_FlexSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_FLEX_SLOT);
        if (f) {
            f->align_self_x = TETHER_ALIGN_FILL;
            f->fill_ratio = 0.0f;
        }
        tether_ui_set_progress(entity, tether_ui_get_progress(entity));
    }

    return true;
}
