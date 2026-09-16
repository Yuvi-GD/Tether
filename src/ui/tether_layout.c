#include "tether_layout.h"
#include "tether/core/tether_components.h"
#include "backends/tether_raster.h"
#include <stdio.h>
#include <stdbool.h>

static void tether_layout_process_entity(Tether_GUID entity, float parent_x, float parent_y, float parent_w, float parent_h);

void tether_layout_process_tree(Tether_GUID root, float screen_width, float screen_height) {
    if (!tether_ecs_is_valid(root)) return;
    tether_layout_process_entity(root, 0.0f, 0.0f, screen_width, screen_height);
}

void tether_layout_process_all(float screen_width, float screen_height) {
    Tether_DenseArray* hierarchies = tether_ecs_get_dense_array(TETHER_COMPONENT_HIERARCHY);
    if (!hierarchies) return;
    
    for (uint32_t i = 0; i < hierarchies->count; i++) {
        Tether_GUID entity = hierarchies->entity_map[i];
        if (!tether_ecs_is_valid(entity)) continue;
        
        Tether_Hierarchy* h = (Tether_Hierarchy*)((uint8_t*)hierarchies->data + (i * hierarchies->element_size));
        /* If entity is a root node (no parent), process it */
        if (!tether_ecs_is_valid(h->parent)) {
            tether_layout_process_tree(entity, screen_width, screen_height);
        }
    }
}

static void tether_layout_process_entity(Tether_GUID entity, float parent_x, float parent_y, float parent_w, float parent_h) {
    if (!tether_ecs_is_valid(entity)) return;

    /* Get required components */
    Tether_SlotTransform* t = (Tether_SlotTransform*)tether_ecs_get_component(entity, TETHER_COMPONENT_SLOT_TRANSFORM);
    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    Tether_LayoutNode* node = (Tether_LayoutNode*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT_NODE);

    if (!t) return; /* Cannot layout an entity without a SlotTransform */

    /* The SlotTransform is populated by the PARENT during the parent's loop. 
       However, for the root entity (or if no parent exists), we use the passed in bounds. */
    if (h && !tether_ecs_is_valid(h->parent)) {
        t->x = parent_x;
        t->y = parent_y;
        t->width = parent_w;
        t->height = parent_h;
    }

    if (!node || !h || h->child_count == 0) return; /* No children to layout */

    /* Available space inside this node (minus padding) */
    float inner_x = t->x + node->padding.left;
    float inner_y = t->y + node->padding.top;
    float inner_w = t->width - node->padding.left - node->padding.right;
    float inner_h = t->height - node->padding.top - node->padding.bottom;
    
    if (inner_w < 0) inner_w = 0;
    if (inner_h < 0) inner_h = 0;

    if (node->flow == TETHER_FLOW_NONE) {
        /* CANVAS MATH (Anchor / Offset) */
        Tether_GUID child = h->first_child;
        while (tether_ecs_is_valid(child)) {
            Tether_SlotTransform* ct = (Tether_SlotTransform*)tether_ecs_get_component(child, TETHER_COMPONENT_SLOT_TRANSFORM);
            Tether_AnchorSlot* anchor = (Tether_AnchorSlot*)tether_ecs_get_component(child, TETHER_COMPONENT_ANCHOR_SLOT);
            
            if (ct && anchor) {
                float min_x = inner_x + (inner_w * anchor->anchor_min.x) + anchor->offset.left;
                float min_y = inner_y + (inner_h * anchor->anchor_min.y) + anchor->offset.top;
                float max_x = inner_x + (inner_w * anchor->anchor_max.x) - anchor->offset.right;
                float max_y = inner_y + (inner_h * anchor->anchor_max.y) - anchor->offset.bottom;
                
                ct->x = min_x;
                ct->y = min_y;
                ct->width = max_x - min_x;
                ct->height = max_y - min_y;
            } else if (ct) {
                /* Fallback if no anchor slot provided: stretch to inner bounds */
                ct->x = inner_x; ct->y = inner_y; ct->width = inner_w; ct->height = inner_h;
            }

            /* Recursive call */
            tether_layout_process_entity(child, inner_x, inner_y, inner_w, inner_h);

            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            child = ch ? ch->next_sibling : 0;
        }
    } else {
        /* FLEX MATH (Row / Column) */
        /* Pass 1: Calculate total fixed size and total fill ratio */
        float total_fill_ratio = 0.0f;
        float total_fixed_space = 0.0f;
        
        Tether_GUID child = h->first_child;
        while (tether_ecs_is_valid(child)) {
            Tether_FlexSlot* flex = (Tether_FlexSlot*)tether_ecs_get_component(child, TETHER_COMPONENT_FLEX_SLOT);
            Tether_TextStyle* text = (Tether_TextStyle*)tether_ecs_get_component(child, TETHER_COMPONENT_TEXT_STYLE);
            if (flex) {
                total_fill_ratio += flex->fill_ratio;
                
                float intrinsic_w = 50.0f;
                float intrinsic_h = 50.0f;
                if (text) {
                    const char* str = tether_ecs_get_text_string(child);
                    if (str && str[0] != '\0') {
                        tether_raster_measure_text(str, text->font_id, text->font_style, text->font_size, &intrinsic_w, &intrinsic_h);
                    }
                }

                if (node->flow == TETHER_FLOW_COLUMN) {
                    total_fixed_space += flex->margin.top + flex->margin.bottom;
                    if (flex->fill_ratio == 0.0f) {
                        total_fixed_space += intrinsic_h; 
                    }
                } else if (node->flow == TETHER_FLOW_ROW) {
                    total_fixed_space += flex->margin.left + flex->margin.right;
                    if (flex->fill_ratio == 0.0f) {
                        total_fixed_space += intrinsic_w; 
                    }
                }
            }
            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            child = ch ? ch->next_sibling : 0;
        }

        /* Pass 2: Distribute space */
        float available_space = 0.0f;
        if (node->flow == TETHER_FLOW_COLUMN) {
            available_space = inner_h - total_fixed_space;
        } else {
            available_space = inner_w - total_fixed_space;
        }
        if (available_space < 0) available_space = 0;

        float current_x = inner_x;
        float current_y = inner_y;

        child = h->first_child;
        while (tether_ecs_is_valid(child)) {
            Tether_SlotTransform* ct = (Tether_SlotTransform*)tether_ecs_get_component(child, TETHER_COMPONENT_SLOT_TRANSFORM);
            Tether_FlexSlot* flex = (Tether_FlexSlot*)tether_ecs_get_component(child, TETHER_COMPONENT_FLEX_SLOT);
            Tether_TextStyle* text = (Tether_TextStyle*)tether_ecs_get_component(child, TETHER_COMPONENT_TEXT_STYLE);
            
            if (ct && flex) {
                float intrinsic_w = 50.0f;
                float intrinsic_h = 50.0f;
                if (text) {
                    const char* str = tether_ecs_get_text_string(child);
                    if (str && str[0] != '\0') {
                        tether_raster_measure_text(str, text->font_id, text->font_style, text->font_size, &intrinsic_w, &intrinsic_h);
                    }
                }

                if (node->flow == TETHER_FLOW_COLUMN) {
                    current_y += flex->margin.top;
                    
                    float item_h = intrinsic_h;
                    if (flex->fill_ratio > 0.0f && total_fill_ratio > 0.0f) {
                        item_h = (flex->fill_ratio / total_fill_ratio) * available_space;
                    }
                    
                    /* Horizontal Alignment */
                    Tether_AlignX align_x = flex->override_align_x ? flex->align_self_x : node->content_align_x;
                    float item_w = inner_w - flex->margin.left - flex->margin.right;
                    float item_x = inner_x + flex->margin.left;
                    
                    if (align_x == TETHER_ALIGN_CENTER) {
                        item_w = text ? intrinsic_w : 100.0f;
                        item_x = inner_x + (inner_w / 2.0f) - (item_w / 2.0f);
                    } else if (align_x == TETHER_ALIGN_LEFT && text) {
                        item_w = intrinsic_w;
                    }
                    
                    ct->x = item_x;
                    ct->y = current_y;
                    ct->width = item_w;
                    ct->height = item_h;
                    
                    current_y += item_h + flex->margin.bottom;
                } else if (node->flow == TETHER_FLOW_ROW) {
                    current_x += flex->margin.left;
                    
                    float item_w = intrinsic_w; 
                    if (flex->fill_ratio > 0.0f && total_fill_ratio > 0.0f) {
                        item_w = (flex->fill_ratio / total_fill_ratio) * available_space;
                    }
                    
                    Tether_AlignY align_y = flex->override_align_y ? flex->align_self_y : node->content_align_y;
                    float item_h = inner_h - flex->margin.top - flex->margin.bottom;
                    float item_y = inner_y + flex->margin.top;
                    
                    if (align_y == TETHER_ALIGN_CENTER) {
                        item_h = text ? intrinsic_h : 100.0f;
                        item_y = inner_y + (inner_h / 2.0f) - (item_h / 2.0f);
                    } else if (align_y == TETHER_ALIGN_TOP && text) {
                        item_h = intrinsic_h;
                    }
                    
                    ct->x = current_x;
                    ct->y = item_y;
                    ct->width = item_w;
                    ct->height = item_h;
                    
                    current_x += item_w + flex->margin.right;
                }
            } else if (ct) {
                /* Fallback if no flex slot provided */
                ct->x = current_x; ct->y = current_y; ct->width = 50.0f; ct->height = 50.0f;
                current_y += 50.0f;
                current_x += 50.0f;
            }

            /* Recursive call */
            tether_layout_process_entity(child, inner_x, inner_y, inner_w, inner_h);

            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            child = ch ? ch->next_sibling : 0;
        }
    }
}
