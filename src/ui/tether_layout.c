#include "tether_layout.h"
#include "tether/core/tether_components.h"
#include "backends/tether_raster.h"
#include <stdbool.h>

static void tether_layout_measure_bottom_up(Tether_GUID entity);
static float tether_layout_arrange_top_down(Tether_GUID entity, float parent_x, float parent_y, float parent_w, float parent_h);

void tether_layout_process_tree(Tether_GUID root, float screen_width, float screen_height) {
    if (!tether_ecs_is_valid(root)) return;
    
    /* Pass 1: Measure (Bottom-Up Intrinsic Sizes) */
    tether_layout_measure_bottom_up(root);
    
    /* Pass 2: Arrange (Top-Down Width + Bottom-Up Height resolution) */
    tether_layout_arrange_top_down(root, 0.0f, 0.0f, screen_width, screen_height);
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

/* Helper: get intrinsic size of a child (text or container) */
static void tether_layout_get_intrinsic_size(Tether_GUID child, float* out_w, float* out_h) {
    Tether_LayoutNode* child_node = (Tether_LayoutNode*)tether_ecs_get_component(child, TETHER_COMPONENT_LAYOUT_NODE);
    if (child_node && child_node->visibility == TETHER_COLLAPSED) {
        *out_w = 0.0f;
        *out_h = 0.0f;
        return;
    }

    Tether_TextStyle* text = (Tether_TextStyle*)tether_ecs_get_component(child, TETHER_COMPONENT_TEXT_STYLE);
    
    float w = 0.0f, h = 0.0f;
    
    if (text) {
        /* Text: measure unbounded single-line to get natural content size */
        const char* str = tether_ecs_get_text_string(child);
        if (str && str[0] != '\0') {
            tether_raster_measure_text(str, text->font_id, text->font_style, text->font_size, text->wrap_width, &w, &h);
            if (child_node) {
                child_node->measured_width = w;
                child_node->measured_height = h;
            }
        }
    } else if (child_node) {
        w = child_node->measured_width;
        h = child_node->measured_height;
    }
    
    *out_w = w;
    *out_h = h;
}

/* 
 * PASS 1: MEASURE (Bottom-Up)
 * Recursively measure children first, then sum up total bounds.
 */
static void tether_layout_measure_bottom_up(Tether_GUID entity) {
    Tether_LayoutNode* node = (Tether_LayoutNode*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT_NODE);
    if (!node) return;
    if (node->visibility == TETHER_COLLAPSED) {
        node->measured_width = 0.0f;
        node->measured_height = 0.0f;
        return;
    }

    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    if (!h) return;
    
    /* Recurse to children first */
    Tether_GUID child = h->first_child;
    while (tether_ecs_is_valid(child)) {
        tether_layout_measure_bottom_up(child);
        
        Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
        child = ch ? ch->next_sibling : TETHER_INVALID_GUID;
    }
        
    /* Base measurement */
    float content_w = 0.0f;
    float content_h = 0.0f;
    
    if (node->flow == TETHER_FLOW_NONE) {
        /* FLOW_NONE: children overlay, content = max child bounding box */
        child = h->first_child;
        while (tether_ecs_is_valid(child)) {
            Tether_LayoutNode* cnode = (Tether_LayoutNode*)tether_ecs_get_component(child, TETHER_COMPONENT_LAYOUT_NODE);
            if (!cnode || cnode->visibility != TETHER_COLLAPSED) {
                float item_w = 0.0f, item_h = 0.0f;
                tether_layout_get_intrinsic_size(child, &item_w, &item_h);
                
                if (item_w > content_w) content_w = item_w;
                if (item_h > content_h) content_h = item_h;
            }
            
            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            child = ch ? ch->next_sibling : TETHER_INVALID_GUID;
        }
    } else {
        /* ROW / COLUMN: children stack along the main axis */
        int child_count = 0;
        child = h->first_child;
        while (tether_ecs_is_valid(child)) {
            Tether_LayoutNode* cnode = (Tether_LayoutNode*)tether_ecs_get_component(child, TETHER_COMPONENT_LAYOUT_NODE);
            if (!cnode || cnode->visibility != TETHER_COLLAPSED) {
                Tether_FlexSlot* flex = (Tether_FlexSlot*)tether_ecs_get_component(child, TETHER_COMPONENT_FLEX_SLOT);
                
                float item_w = 0.0f, item_h = 0.0f;
                tether_layout_get_intrinsic_size(child, &item_w, &item_h);
                
                /* Overrides */
                if (flex) {
                    if (flex->explicit_size.x > 0.0f) item_w = flex->explicit_size.x;
                    if (flex->explicit_size.y > 0.0f) item_h = flex->explicit_size.y;
                    
                    item_w += flex->margin.left + flex->margin.right;
                    item_h += flex->margin.top + flex->margin.bottom;
                }
                
                if (node->flow == TETHER_FLOW_ROW) {
                    content_w += item_w;
                    if (item_h > content_h) content_h = item_h;
                    
                    if (child_count > 0) content_w += node->gap.x;
                } else { /* COLUMN */
                    content_h += item_h;
                    if (item_w > content_w) content_w = item_w;
                    
                    if (child_count > 0) content_h += node->gap.y;
                }
                
                child_count++;
            }
            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            child = ch ? ch->next_sibling : TETHER_INVALID_GUID;
        }
    }
    
    node->measured_width = content_w + node->padding.left + node->padding.right;
    node->measured_height = content_h + node->padding.top + node->padding.bottom;
}

/* 
 * PASS 2: ARRANGE (Top-Down Width + Bottom-Up Height)
 * Returns the final calculated height of the entity.
 */
static float tether_layout_arrange_top_down(Tether_GUID entity, float parent_x, float parent_y, float parent_w, float parent_h) {
    Tether_SlotTransform* t = (Tether_SlotTransform*)tether_ecs_get_component(entity, TETHER_COMPONENT_SLOT_TRANSFORM);
    Tether_Hierarchy* h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    Tether_LayoutNode* node = (Tether_LayoutNode*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT_NODE);
    Tether_TextStyle* text = (Tether_TextStyle*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_STYLE);
    Tether_FlexSlot* my_flex = (Tether_FlexSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_FLEX_SLOT);

    if (!t) return 0.0f;

    if (node && node->visibility == TETHER_COLLAPSED) {
        t->x = 0.0f;
        t->y = 0.0f;
        t->width = 0.0f;
        t->height = 0.0f;
        return 0.0f;
    }

    t->x = parent_x;
    t->y = parent_y;
    t->width = parent_w;
    t->height = parent_h;

    bool is_auto_height = false;
    /* Determine if we should auto-size our height based on content */
    if (my_flex && my_flex->fill_ratio == 0.0f && my_flex->explicit_size.y <= 0.0f) {
        is_auto_height = true;
        
        /* If our parent is a ROW and told us to stretch vertically, we MUST NOT shrink wrap! */
        if (h && tether_ecs_is_valid(h->parent)) {
            Tether_LayoutNode* pnode = (Tether_LayoutNode*)tether_ecs_get_component(h->parent, TETHER_COMPONENT_LAYOUT_NODE);
            if (pnode && pnode->flow == TETHER_FLOW_ROW) {
                Tether_Align align = my_flex->override_align_y ? my_flex->align_self_y : pnode->content_align_y;
                if (align == TETHER_ALIGN_FILL) {
                    is_auto_height = false;
                }
            }
        }
    }
    
    /* Root nodes always take their provided bounds (screen bounds) */
    if (!h || !tether_ecs_is_valid(h->parent)) {
        is_auto_height = false;
    }

    /* LEAF NODE: TEXT */
    if (text) {
        const char* str = tether_ecs_get_text_string(entity);
        if (str && str[0] != '\0') {
            float max_w = 0.0f;
            float wrapped_w = 0.0f, wrapped_h = 0.0f;
            if (text->wrap_width > 0.0f) {
                max_w = (t->width < text->wrap_width) ? t->width : text->wrap_width;
                t->width = max_w;
                if (node) node->wrap = 1;
            } else if (node && t->width < (node->measured_width - 0.5f)) {
                max_w = t->width;
                node->wrap = 1;
            } else if (node) {
                node->wrap = 0;
            }
            
            tether_raster_measure_text(str, text->font_id, text->font_style, text->font_size, max_w, &wrapped_w, &wrapped_h);
            if (is_auto_height) {
                t->height = wrapped_h;
            }
        }
        return t->height;
    }

    /* CONTAINER NODE */
    if (!node || !h || h->child_count == 0) return t->height;

    float inner_x = t->x + node->padding.left;
    float inner_y = t->y + node->padding.top;
    float inner_w = t->width - node->padding.left - node->padding.right;
    float inner_h = t->height - node->padding.top - node->padding.bottom;
    
    if (inner_w < 0) inner_w = 0;
    if (inner_h < 0) inner_h = 0;

    float actual_content_h = 0.0f;

    if (node->flow == TETHER_FLOW_NONE) {
        /* Absolute positioning via anchors */
        Tether_GUID child = h->first_child;
        while (tether_ecs_is_valid(child)) {
            Tether_LayoutNode* cnode = (Tether_LayoutNode*)tether_ecs_get_component(child, TETHER_COMPONENT_LAYOUT_NODE);
            if (!cnode || cnode->visibility != TETHER_COLLAPSED) {
                Tether_AnchorSlot* anchor = (Tether_AnchorSlot*)tether_ecs_get_component(child, TETHER_COMPONENT_ANCHOR_SLOT);
                
                float cx = inner_x, cy = inner_y, cw = inner_w, ch = inner_h;
                
                if (anchor) {
                    float min_x = inner_x + (inner_w * anchor->anchor_min.x) + anchor->offset.left;
                    float min_y = inner_y + (inner_h * anchor->anchor_min.y) + anchor->offset.top;
                    float max_x = inner_x + (inner_w * anchor->anchor_max.x) - anchor->offset.right;
                    float max_y = inner_y + (inner_h * anchor->anchor_max.y) - anchor->offset.bottom;
                    
                    cx = min_x; cy = min_y; cw = max_x - min_x; ch = max_y - min_y;
                }
                
                float child_used_h = tether_layout_arrange_top_down(child, cx, cy, cw, ch);
                if (child_used_h > actual_content_h) actual_content_h = child_used_h;
            } else {
                tether_layout_arrange_top_down(child, 0, 0, 0, 0); /* Let it zero itself */
            }
            
            Tether_Hierarchy* ch_h = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            child = ch_h ? ch_h->next_sibling : TETHER_INVALID_GUID;
        }
    } else {
        /* ---- ROW / COLUMN ---- */
        
        /* 
         * Counting pass: sum up fixed space and fill ratios.
         */
        float total_fill_ratio = 0.0f;
        float fixed_space = 0.0f;
        int child_count = 0;
        
        Tether_GUID child = h->first_child;
        while (tether_ecs_is_valid(child)) {
            Tether_LayoutNode* cnode = (Tether_LayoutNode*)tether_ecs_get_component(child, TETHER_COMPONENT_LAYOUT_NODE);
            if (!cnode || cnode->visibility != TETHER_COLLAPSED) {
                Tether_FlexSlot* flex = (Tether_FlexSlot*)tether_ecs_get_component(child, TETHER_COMPONENT_FLEX_SLOT);
                
                float intrinsic_w = 0.0f, intrinsic_h = 0.0f;
                tether_layout_get_intrinsic_size(child, &intrinsic_w, &intrinsic_h);
                
                if (flex) {
                    if (flex->explicit_size.x > 0.0f) intrinsic_w = flex->explicit_size.x;
                    if (flex->explicit_size.y > 0.0f) intrinsic_h = flex->explicit_size.y;
                    
                    total_fill_ratio += flex->fill_ratio;
                    
                    if (node->flow == TETHER_FLOW_ROW) {
                        fixed_space += flex->margin.left + flex->margin.right;
                        if (flex->fill_ratio == 0.0f) fixed_space += intrinsic_w;
                    } else {
                        fixed_space += flex->margin.top + flex->margin.bottom;
                        if (flex->fill_ratio == 0.0f) fixed_space += intrinsic_h;
                    }
                }
                
                child_count++;
            }
            Tether_Hierarchy* ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            child = ch ? ch->next_sibling : TETHER_INVALID_GUID;
        }
        
        if (child_count > 1) {
            if (node->flow == TETHER_FLOW_ROW) fixed_space += node->gap.x * (child_count - 1);
            else fixed_space += node->gap.y * (child_count - 1);
        }
        
        float available_space = (node->flow == TETHER_FLOW_ROW ? inner_w : inner_h) - fixed_space;
        if (available_space < 0) available_space = 0;
        
        /* 
         * Placement pass: assign final bounds to each child and resolve actual heights.
         */
        float current_x = inner_x;
        float current_y = inner_y;
        
        child = h->first_child;
        while (tether_ecs_is_valid(child)) {
            Tether_LayoutNode* cnode = (Tether_LayoutNode*)tether_ecs_get_component(child, TETHER_COMPONENT_LAYOUT_NODE);
            if (!cnode || cnode->visibility != TETHER_COLLAPSED) {
                Tether_FlexSlot* flex = (Tether_FlexSlot*)tether_ecs_get_component(child, TETHER_COMPONENT_FLEX_SLOT);
                
                float intrinsic_w = 0.0f, intrinsic_h = 0.0f;
                tether_layout_get_intrinsic_size(child, &intrinsic_w, &intrinsic_h);
                
                float item_w = intrinsic_w;
                float item_h = intrinsic_h;
                
                float cx = current_x, cy = current_y, cw = item_w, ch = item_h;
                
                if (flex) {
                    if (flex->explicit_size.x > 0.0f) item_w = flex->explicit_size.x;
                    if (flex->explicit_size.y > 0.0f) item_h = flex->explicit_size.y;
                    
                    if (node->flow == TETHER_FLOW_ROW) {
                        current_x += flex->margin.left;
                        
                        /* Flex children share remaining space by ratio */
                        if (flex->fill_ratio > 0.0f && total_fill_ratio > 0.0f) {
                            item_w = (flex->fill_ratio / total_fill_ratio) * available_space;
                        }
                        
                        /* Cross-axis (vertical) alignment */
                        Tether_Align align = flex->override_align_y ? flex->align_self_y : node->content_align_y;
                        if (align == TETHER_ALIGN_FILL) {
                            ch = inner_h - flex->margin.top - flex->margin.bottom;
                        } else if (align == TETHER_ALIGN_CENTER) {
                            ch = item_h;
                            cy = inner_y + (inner_h / 2.0f) - (ch / 2.0f);
                        } else { // TOP or BOTTOM
                            ch = item_h;
                        }
                        
                        cw = item_w;
                        cx = current_x;
                        
                        float child_used_h = tether_layout_arrange_top_down(child, cx, cy, cw, ch);
                        
                        if (flex->fill_ratio == 0.0f && flex->explicit_size.y <= 0.0f && align != TETHER_ALIGN_FILL) {
                            ch = child_used_h;
                        }
                        
                        if (ch > actual_content_h) actual_content_h = ch;
                        
                        current_x += cw + flex->margin.right + node->gap.x;
                    } else { /* COLUMN */
                        current_y += flex->margin.top;
                        
                        /* Flex children share remaining space by ratio */
                        if (flex->fill_ratio > 0.0f && total_fill_ratio > 0.0f) {
                            item_h = (flex->fill_ratio / total_fill_ratio) * available_space;
                        }
                        
                        /* Cross-axis (horizontal) alignment */
                        Tether_Align align = flex->override_align_x ? flex->align_self_x : node->content_align_x;
                        if (align == TETHER_ALIGN_FILL) {
                            cw = inner_w - flex->margin.left - flex->margin.right;
                            cx = inner_x + flex->margin.left;
                        } else if (align == TETHER_ALIGN_CENTER) {
                            cw = item_w;
                            cx = inner_x + (inner_w / 2.0f) - (cw / 2.0f);
                        } else { // LEFT or RIGHT
                            cw = item_w;
                            cx = inner_x + flex->margin.left;
                        }
                        
                        ch = item_h;
                        cy = current_y;
                        
                        float child_used_h = tether_layout_arrange_top_down(child, cx, cy, cw, ch);
                        
                        /* In a Column, an auto-sized child tells us how far to advance Y! */
                        if (flex->fill_ratio == 0.0f && flex->explicit_size.y <= 0.0f) {
                            ch = child_used_h;
                        }
                        
                        current_y += ch + flex->margin.bottom + node->gap.y;
                    }
                } else {
                    tether_layout_arrange_top_down(child, cx, cy, cw, ch);
                }
            } else {
                tether_layout_arrange_top_down(child, 0, 0, 0, 0); /* Let it zero itself */
            }
            
            Tether_Hierarchy* ch_h = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            child = ch_h ? ch_h->next_sibling : TETHER_INVALID_GUID;
        }
        
        if (node->flow == TETHER_FLOW_COLUMN) {
            actual_content_h = current_y - inner_y;
            if (h->first_child != TETHER_INVALID_GUID) {
                actual_content_h -= node->gap.y;
            }
        }
    }
    
    if (is_auto_height) {
        t->height = actual_content_h + node->padding.top + node->padding.bottom;
    }
    
    return t->height;
}
