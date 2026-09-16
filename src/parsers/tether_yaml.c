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
    node->content_align_y = TETHER_ALIGN_Y_FILL;
    node->padding_top = 0; node->padding_bottom = 0; node->padding_left = 0; node->padding_right = 0;

    Tether_AnchorSlot* anchor = (Tether_AnchorSlot*)tether_ecs_add_component(entity, TETHER_COMPONENT_ANCHOR_SLOT);
    anchor->anchor_min_x = 0; anchor->anchor_min_y = 0;
    anchor->anchor_max_x = 1; anchor->anchor_max_y = 1;
    anchor->offset_top = 0; anchor->offset_bottom = 0; anchor->offset_left = 0; anchor->offset_right = 0;

    Tether_FlexSlot* flex = (Tether_FlexSlot*)tether_ecs_add_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    flex->margin_top = 0; flex->margin_bottom = 0; flex->margin_left = 0; flex->margin_right = 0;
    flex->fill_ratio = 1.0f;
    flex->override_align_x = 0; flex->override_align_y = 0;

    Tether_RenderTransform* rt = (Tether_RenderTransform*)tether_ecs_add_component(entity, TETHER_COMPONENT_RENDER_TRANSFORM);
    rt->translation_x = 0; rt->translation_y = 0; rt->scale_x = 1.0f; rt->scale_y = 1.0f;
    rt->rotation_deg = 0; rt->pivot_x = 0.5f; rt->pivot_y = 0.5f;

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
    t->align_x = TETHER_ALIGN_LEFT;
    t->align_y = TETHER_ALIGN_TOP;
    
    /* Text defaults to shrink-wrap layout */
    Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    if (f) f->fill_ratio = 0.0f;
    
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
        STATE_DYNAMIC
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
                else if (strcmp(value, "dynamic") == 0) { state = STATE_DYNAMIC; }
                else if (stack_idx >= 0) {
                    /* Read Values */
                    Tether_GUID ent = entity_stack[stack_idx];
                    
                    if (state == STATE_COLOR) {
                        Tether_Color* c = (Tether_Color*)tether_ecs_get_component(ent, TETHER_COMPONENT_COLOR);
                        int v = atoi(value);
                        if (array_idx == 0) c->r = v;
                        else if (array_idx == 1) c->g = v;
                        else if (array_idx == 2) c->b = v;
                        else if (array_idx == 3) c->a = v;
                        array_idx++;
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
                        if (array_idx == 0) a->anchor_min_x = v;
                        else if (array_idx == 1) a->anchor_min_y = v;
                        array_idx++;
                    }
                    else if (state == STATE_ANCHOR_MAX) {
                        Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
                        float v = strtof(value, NULL);
                        if (array_idx == 0) a->anchor_max_x = v;
                        else if (array_idx == 1) a->anchor_max_y = v;
                        array_idx++;
                    }
                    else if (state == STATE_OFFSET) {
                        Tether_AnchorSlot* a = (Tether_AnchorSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_ANCHOR_SLOT);
                        float v = strtof(value, NULL);
                        if (array_idx == 0) a->offset_top = v;
                        else if (array_idx == 1) a->offset_bottom = v;
                        else if (array_idx == 2) a->offset_left = v;
                        else if (array_idx == 3) a->offset_right = v;
                        array_idx++;
                    }
                    else if (state == STATE_MARGIN) {
                        Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(ent, TETHER_COMPONENT_FLEX_SLOT);
                        float v = strtof(value, NULL);
                        if (array_idx == 0) f->margin_top = v;
                        else if (array_idx == 1) f->margin_bottom = v;
                        else if (array_idx == 2) f->margin_left = v;
                        else if (array_idx == 3) f->margin_right = v;
                        array_idx++;
                    }
                    else if (state == STATE_PADDING) {
                        Tether_LayoutNode* n = (Tether_LayoutNode*)tether_ecs_get_component(ent, TETHER_COMPONENT_LAYOUT_NODE);
                        float v = strtof(value, NULL);
                        if (array_idx == 0) n->padding_top = v;
                        else if (array_idx == 1) n->padding_bottom = v;
                        else if (array_idx == 2) n->padding_left = v;
                        else if (array_idx == 3) n->padding_right = v;
                        array_idx++;
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
