#include "parsers/tether_yaml.h"
#include "tether/core/tether_components.h"
#include <yaml.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Tether_GUID create_panel(Tether_GUID parent) {
    Tether_GUID entity = tether_ecs_create_entity();
    
    /* Output: SlotTransform */
    Tether_SlotTransform* st = (Tether_SlotTransform*)tether_ecs_add_component(entity, TETHER_COMPONENT_SLOT_TRANSFORM);
    st->x = 0; st->y = 0; st->width = 100; st->height = 100;
    
    /* Defaults */
    Tether_LayoutNode* node = (Tether_LayoutNode*)tether_ecs_add_component(entity, TETHER_COMPONENT_LAYOUT_NODE);
    node->flow = TETHER_FLOW_NONE;
    node->content_align_x = TETHER_ALIGN_FILL;
    node->content_align_y = TETHER_ALIGN_FILL;
    node->padding.top = 0; node->padding.bottom = 0; node->padding.left = 0; node->padding.right = 0;
    node->gap.x = 0; node->gap.y = 0;
    node->wrap = 0;
    node->hit_behavior = TETHER_HIT_BLOCK; /* Panels catch hits by default */
    node->id[0] = '\0';
    node->hit_behavior = TETHER_HIT_BLOCK; /* Panels catch hits by default */

    Tether_AnchorSlot* anchor = (Tether_AnchorSlot*)tether_ecs_add_component(entity, TETHER_COMPONENT_ANCHOR_SLOT);
    anchor->anchor_min.x = 0; anchor->anchor_min.y = 0;
    anchor->anchor_max.x = 1; anchor->anchor_max.y = 1;
    anchor->offset.top = 0; anchor->offset.bottom = 0; anchor->offset.left = 0; anchor->offset.right = 0;

    Tether_FlexSlot* flex = (Tether_FlexSlot*)tether_ecs_add_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    flex->margin.top = 0; flex->margin.bottom = 0; flex->margin.left = 0; flex->margin.right = 0;
    flex->explicit_size.x = 0; flex->explicit_size.y = 0;
    flex->fill_ratio = 0.0f;
    flex->override_align_x = 0; flex->override_align_y = 0;

    Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_add_component(entity, TETHER_COMPONENT_RENDER_TRANSFORM);
    rt->translation_x = 0; rt->translation_y = 0; rt->scale_x = 1.0f; rt->scale_y = 1.0f;
    rt->rotation_deg = 0; rt->pivot_x = 0.5f; rt->pivot_y = 0.5f;

    Tether_Style* s = (Tether_Style*)tether_ecs_add_component(entity, TETHER_COMPONENT_STYLE);
    if (s) {
        s->bg_color.r = 255; s->bg_color.g = 255; s->bg_color.b = 255; s->bg_color.a = 255;
        s->border_color.r = 0; s->border_color.g = 0; s->border_color.b = 0; s->border_color.a = 0;
        s->border_width = 0.0f;
        s->border_radius.top = 0.0f; s->border_radius.right = 0.0f;
        s->border_radius.bottom = 0.0f; s->border_radius.left = 0.0f;
    }

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
                ph->last_child = entity;
            } else {
                Tether_GUID old_last = ph->last_child;
                Tether_Hierarchy* old_last_h = (Tether_Hierarchy*)tether_ecs_get_component(old_last, TETHER_COMPONENT_HIERARCHY);
                if (old_last_h) {
                    old_last_h->next_sibling = entity;
                    h->prev_sibling = old_last;
                }
                ph->last_child = entity;
            }
        }
    }

    return entity;
}

static Tether_GUID create_text_node(Tether_GUID parent) {
    Tether_GUID entity = create_panel(parent);
    
    Tether_TextStyle* t = (Tether_TextStyle*)tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT_STYLE);
    t->font_size = 24.0f;
    t->font_id = 0;
    t->font_style = TETHER_FONT_NORMAL;
    t->align_x = TETHER_ALIGN_START;
    t->align_y = TETHER_ALIGN_START;
    t->wrap_width = 0.0f;
    
    /* Text defaults to shrink-wrap layout */
    Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    if (f) f->fill_ratio = 0.0f;
    
    /* Text defaults to ignoring hits so it doesn't block buttons */
    Tether_LayoutNode* node = (Tether_LayoutNode*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT_NODE);
    if (node) node->hit_behavior = TETHER_HIT_IGNORE_SELF;
    
    return entity;
}

/* Very simple YAML parser for Sandbox UI */
Tether_GUID tether_yaml_load(const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file) return TETHER_INVALID_GUID;

    yaml_parser_t parser;
    yaml_event_t event;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, file);

    Tether_GUID root = TETHER_INVALID_GUID;
    Tether_GUID entity_stack[64];
    int stack_idx = -1;
    
    enum { 
        STATE_NONE, 
        STATE_COLOR, 
        STATE_CHILDREN,
        STATE_FLOW,
        STATE_FILL_RATIO,
        STATE_ANCHOR_MIN,
        STATE_ANCHOR_MAX,
        STATE_OFFSET,
        STATE_MARGIN,
        STATE_PADDING,
        STATE_TEXT_STRING,
        STATE_FONT_SIZE,
        STATE_DYNAMIC,
        STATE_GAP,
        STATE_WRAP,
        STATE_WRAP_WIDTH,
        STATE_EXPLICIT_SIZE,
        STATE_ID,
        STATE_HIT_BEHAVIOR,
        STATE_INTERACTABLE,
        STATE_HOVER_COLOR,
        STATE_PRESS_COLOR,
        STATE_VISIBILITY,
        STATE_ALIGN_X,
        STATE_ALIGN_Y
    } state = STATE_NONE;
    int array_idx = 0;
    int mapping_depth = 0;

    while (yaml_parser_parse(&parser, &event)) {
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
                    stack_idx--; /* Pop entity */
                }
                break;
                
            case YAML_SEQUENCE_END_EVENT:
                state = STATE_NONE;
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
                else if (strcmp(value, "Text") == 0) {
                    Tether_GUID parent = stack_idx >= 0 ? entity_stack[stack_idx] : TETHER_INVALID_GUID;
                    Tether_GUID new_ent = create_text_node(parent);
                    if (root == TETHER_INVALID_GUID) root = new_ent;
                    entity_stack[++stack_idx] = new_ent;
                    state = STATE_NONE;
                }
                else if (strcmp(value, "color") == 0) { state = STATE_COLOR; array_idx = 0; }
                else if (strcmp(value, "children") == 0) { state = STATE_CHILDREN; }
                else if (strcmp(value, "flow") == 0) { state = STATE_FLOW; }
                else if (strcmp(value, "fill_ratio") == 0) { state = STATE_FILL_RATIO; }
                else if (strcmp(value, "anchor_min") == 0) { state = STATE_ANCHOR_MIN; array_idx = 0; }
                else if (strcmp(value, "anchor_max") == 0) { state = STATE_ANCHOR_MAX; array_idx = 0; }
                else if (strcmp(value, "offset") == 0) { state = STATE_OFFSET; array_idx = 0; }
                else if (strcmp(value, "margin") == 0) { state = STATE_MARGIN; array_idx = 0; }
                else if (strcmp(value, "padding") == 0) { state = STATE_PADDING; array_idx = 0; }
                else if (strcmp(value, "string") == 0) { state = STATE_TEXT_STRING; }
                else if (strcmp(value, "font_size") == 0) { state = STATE_FONT_SIZE; }
                else if (strcmp(value, "wrap_width") == 0) { state = STATE_WRAP_WIDTH; }
                else if (strcmp(value, "dynamic") == 0) { state = STATE_DYNAMIC; }
                else if (strcmp(value, "gap") == 0) { state = STATE_GAP; array_idx = 0; }
                else if (strcmp(value, "wrap") == 0) { state = STATE_WRAP; }
                else if (strcmp(value, "explicit_size") == 0) { state = STATE_EXPLICIT_SIZE; array_idx = 0; }
                else if (strcmp(value, "id") == 0) { state = STATE_ID; }
                else if (strcmp(value, "hit_behavior") == 0) { state = STATE_HIT_BEHAVIOR; }
                else if (strcmp(value, "visibility") == 0) { state = STATE_VISIBILITY; }
                else if (strcmp(value, "align_x") == 0) { state = STATE_ALIGN_X; }
                else if (strcmp(value, "align_y") == 0) { state = STATE_ALIGN_Y; }
                else if (strcmp(value, "interactable") == 0) { state = STATE_INTERACTABLE; }
                else if (strcmp(value, "hover_color") == 0) { state = STATE_HOVER_COLOR; array_idx = 0; }
                else if (strcmp(value, "press_color") == 0) { state = STATE_PRESS_COLOR; array_idx = 0; }
                else if (stack_idx >= 0) {
                    /* Read Values */
                    Tether_GUID ent = entity_stack[stack_idx];
                    
                    if (state == STATE_COLOR) {
                        Tether_Style* s = (Tether_Style*)tether_ecs_get_component(ent, TETHER_COMPONENT_STYLE);
                        if (!s) s = (Tether_Style*)tether_ecs_add_component(ent, TETHER_COMPONENT_STYLE);
                        int v = atoi(value);
                        if (array_idx == 0) s->bg_color.r = v;
                        else if (array_idx == 1) s->bg_color.g = v;
                        else if (array_idx == 2) s->bg_color.b = v;
                        else if (array_idx == 3) s->bg_color.a = v;
                        array_idx++;
                    }
                    else if (state == STATE_HOVER_COLOR) {
                        Tether_Style* s = (Tether_Style*)tether_ecs_get_component(ent, TETHER_COMPONENT_STYLE);
                        if (!s) s = (Tether_Style*)tether_ecs_add_component(ent, TETHER_COMPONENT_STYLE);
                        if (strcmp(value, "AUTO") == 0) {
                            s->hover_color_mode = TETHER_COLOR_MODE_AUTO;
                            state = STATE_NONE;
                        } else {
                            s->hover_color_mode = TETHER_COLOR_MODE_MANUAL;
                            int v = atoi(value);
                            if (array_idx == 0) s->hover_color.r = v;
                            else if (array_idx == 1) s->hover_color.g = v;
                            else if (array_idx == 2) s->hover_color.b = v;
                            else if (array_idx == 3) { s->hover_color.a = v; state = STATE_NONE; }
                            array_idx++;
                        }
                    }
                    else if (state == STATE_PRESS_COLOR) {
                        Tether_Style* s = (Tether_Style*)tether_ecs_get_component(ent, TETHER_COMPONENT_STYLE);
                        if (!s) s = (Tether_Style*)tether_ecs_add_component(ent, TETHER_COMPONENT_STYLE);
                        if (strcmp(value, "AUTO") == 0) {
                            s->press_color_mode = TETHER_COLOR_MODE_AUTO;
                            state = STATE_NONE;
                        } else {
                            s->press_color_mode = TETHER_COLOR_MODE_MANUAL;
                            int v = atoi(value);
                            if (array_idx == 0) s->press_color.r = v;
                            else if (array_idx == 1) s->press_color.g = v;
                            else if (array_idx == 2) s->press_color.b = v;
                            else if (array_idx == 3) { s->press_color.a = v; state = STATE_NONE; }
                            array_idx++;
                        }
                    }
                    else if (state == STATE_FLOW) {
                        Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
                        if (strcmp(value, "ROW") == 0) n->flow = TETHER_FLOW_ROW;
                        else if (strcmp(value, "COLUMN") == 0) n->flow = TETHER_FLOW_COLUMN;
                        else n->flow = TETHER_FLOW_NONE;
                        state = STATE_NONE;
                    }
                    else if (state == STATE_FILL_RATIO) {
                        Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_FLEX_SLOT);
                        f->fill_ratio = strtof(value, NULL);
                        state = STATE_NONE;
                    }
                    else if (state == STATE_ANCHOR_MIN) {
                        Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
                        float v = strtof(value, NULL);
                        if (array_idx == 0) a->anchor_min.x = v;
                        else if (array_idx == 1) a->anchor_min.y = v;
                        array_idx++;
                    }
                    else if (state == STATE_ANCHOR_MAX) {
                        Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
                        float v = strtof(value, NULL);
                        if (array_idx == 0) a->anchor_max.x = v;
                        else if (array_idx == 1) a->anchor_max.y = v;
                        array_idx++;
                    }
                    else if (state == STATE_OFFSET) {
                        Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
                        if (array_idx < 4) {
                            float* arr = (float*)&a->offset;
                            arr[array_idx] = strtof(value, NULL);
                        }
                        array_idx++;
                    }
                    else if (state == STATE_MARGIN) {
                        Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_FLEX_SLOT);
                        if (array_idx < 4) {
                            float* arr = (float*)&f->margin;
                            arr[array_idx] = strtof(value, NULL);
                        }
                        array_idx++;
                    }
                    else if (state == STATE_PADDING) {
                        Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
                        if (array_idx < 4) {
                            float* arr = (float*)&n->padding;
                            arr[array_idx] = strtof(value, NULL);
                        }
                        array_idx++;
                    }
                    else if (state == STATE_GAP) {
                        Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
                        float v = strtof(value, NULL);
                        if (array_idx == 0) n->gap.x = v;
                        else if (array_idx == 1) n->gap.y = v;
                        array_idx++;
                    }
                    else if (state == STATE_WRAP) {
                        Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
                        if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) n->wrap = 1;
                        else n->wrap = 0;
                        state = STATE_NONE;
                    }
                    else if (state == STATE_EXPLICIT_SIZE) {
                        Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_FLEX_SLOT);
                        float v = strtof(value, NULL);
                        if (array_idx == 0) f->explicit_size.x = v;
                        else if (array_idx == 1) f->explicit_size.y = v;
                        array_idx++;
                    }
                    else if (state == STATE_ID) {
                        Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
                        if (n) {
                            strncpy(n->id, value, 31);
                            n->id[31] = '\0';
                        }
                        state = STATE_NONE;
                    }
                    else if (state == STATE_HIT_BEHAVIOR) {
                        Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
                        if (n) {
                            if (strcmp(value, "BLOCK") == 0) n->hit_behavior = TETHER_HIT_BLOCK;
                            else if (strcmp(value, "IGNORE_SELF") == 0) n->hit_behavior = TETHER_HIT_IGNORE_SELF;
                            else if (strcmp(value, "IGNORE_ALL") == 0) n->hit_behavior = TETHER_HIT_IGNORE_ALL;
                        }
                        state = STATE_NONE;
                    }
                    else if (state == STATE_VISIBILITY) {
                        Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
                        if (n) {
                            if (strcmp(value, "hidden") == 0) n->visibility = TETHER_HIDDEN;
                            else if (strcmp(value, "collapsed") == 0) n->visibility = TETHER_COLLAPSED;
                            else n->visibility = TETHER_VISIBLE;
                        }
                        state = STATE_NONE;
                    }
                    else if (state == STATE_ALIGN_X) {
                        Tether_TextStyle* t = (Tether_TextStyle*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT_STYLE);
                        if (t) {
                            if (strcmp(value, "fill") == 0) t->align_x = TETHER_ALIGN_FILL;
                            else if (strcmp(value, "center") == 0) t->align_x = TETHER_ALIGN_CENTER;
                            else if (strcmp(value, "end") == 0 || strcmp(value, "right") == 0) t->align_x = TETHER_ALIGN_END;
                            else t->align_x = TETHER_ALIGN_START;
                        }
                        state = STATE_NONE;
                    }
                    else if (state == STATE_ALIGN_Y) {
                        Tether_TextStyle* t = (Tether_TextStyle*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT_STYLE);
                        if (t) {
                            if (strcmp(value, "fill") == 0) t->align_y = TETHER_ALIGN_FILL;
                            else if (strcmp(value, "center") == 0) t->align_y = TETHER_ALIGN_CENTER;
                            else if (strcmp(value, "end") == 0 || strcmp(value, "bottom") == 0) t->align_y = TETHER_ALIGN_END;
                            else t->align_y = TETHER_ALIGN_START;
                        }
                        state = STATE_NONE;
                    }
                    else if (state == STATE_INTERACTABLE) {
                        if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                            tether_ecs_add_component(ent, TETHER_COMPONENT_INTERACTABLE);
                        }
                        state = STATE_NONE;
                    }
                    else if (state == STATE_TEXT_STRING) {
                        size_t len = strlen(value);
                        Tether_TextDynamic* dyn = (Tether_TextDynamic*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT_DYNAMIC);
                        
                        if (dyn || len > 511) {
                            if (!dyn) dyn = (Tether_TextDynamic*)tether_ecs_add_component(ent, TETHER_COMPONENT_TEXT_DYNAMIC);
                            dyn->capacity = (uint32_t)(len + 1) * 2;
                            dyn->data = (char*)malloc(dyn->capacity);
                            strcpy(dyn->data, value);
                            dyn->length = (uint32_t)len;
                        } else if (len <= 31) {
                            Tether_TextWord* t = (Tether_TextWord*)tether_ecs_add_component(ent, TETHER_COMPONENT_TEXT_WORD);
                            strncpy(t->data, value, 31);
                            t->data[31] = '\0';
                        } else if (len <= 127) {
                            Tether_TextLabel* t = (Tether_TextLabel*)tether_ecs_add_component(ent, TETHER_COMPONENT_TEXT_LABEL);
                            strncpy(t->data, value, 127);
                            t->data[127] = '\0';
                        } else if (len <= 511) {
                            Tether_TextParagraph* t = (Tether_TextParagraph*)tether_ecs_add_component(ent, TETHER_COMPONENT_TEXT_PARAGRAPH);
                            strncpy(t->data, value, 511);
                            t->data[511] = '\0';
                        }
                        state = STATE_NONE;
                    }
                    else if (state == STATE_FONT_SIZE) {
                        Tether_TextStyle* t = (Tether_TextStyle*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT_STYLE);
                        if (t) t->font_size = strtof(value, NULL);
                        state = STATE_NONE;
                    }
                    else if (state == STATE_WRAP_WIDTH) {
                        Tether_TextStyle* t = (Tether_TextStyle*)tether_ecs_get_component(ent, TETHER_COMPONENT_TEXT_STYLE);
                        if (t) t->wrap_width = strtof(value, NULL);
                        state = STATE_NONE;
                    }
                    else if (state == STATE_DYNAMIC) {
                        if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                            /* Pre-add the component to mark it as dynamic for when the string is parsed */
                            tether_ecs_add_component(ent, TETHER_COMPONENT_TEXT_DYNAMIC);
                        }
                        state = STATE_NONE;
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
