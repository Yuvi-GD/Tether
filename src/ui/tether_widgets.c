#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"

Tether_GUID tether_widget_create_panel(Tether_GUID parent) {
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
    node->visibility = TETHER_VISIBLE;

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
        s->bg_color = (Tether_Color){0, 0, 0, 0};
        s->border_color.r = 0; s->border_color.g = 0; s->border_color.b = 0; s->border_color.a = 0;
        s->border_width = 0.0f;
        s->border_radius.top = 0.0f; s->border_radius.right = 0.0f;
        s->border_radius.bottom = 0.0f; s->border_radius.left = 0.0f;
        s->hover_color_mode = TETHER_COLOR_MODE_NONE;
        s->press_color_mode = TETHER_COLOR_MODE_NONE;
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

Tether_GUID tether_widget_create_text(Tether_GUID parent) {
    Tether_GUID entity = tether_widget_create_panel(parent);

    Tether_TextStyle* t = (Tether_TextStyle*)tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT_STYLE);
    t->font_size = 24.0f;
    t->font_id = 0;
    t->font_style = TETHER_FONT_NORMAL;
    t->align_x = TETHER_ALIGN_START;
    t->align_y = TETHER_ALIGN_START;
    t->wrap_width = 0.0f;
    
    /* Text widgets need a place to store their string data */
    /* Text defaults to shrink-wrap layout */
    Tether_FlexSlot* f = (Tether_FlexSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    if (f) f->fill_ratio = 0.0f;
    
    /* Text defaults to ignoring hits so it doesn't block buttons */
    Tether_LayoutNode* node = (Tether_LayoutNode*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT_NODE);
    if (node) node->hit_behavior = TETHER_HIT_IGNORE_SELF;
    
    /* Default text color to black (can be overridden via TETHER_COMPONENT_STYLE) */
    Tether_Style* s = (Tether_Style*)tether_ecs_get_component(entity, TETHER_COMPONENT_STYLE);
    if (s) {
        s->bg_color = (Tether_Color){0, 0, 0, 255};
    }
    
    return entity;
}

Tether_GUID tether_widget_create_button(Tether_GUID parent) {
    Tether_GUID entity = tether_widget_create_panel(parent);

    Tether_LayoutNode* node = (Tether_LayoutNode*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT_NODE);
    if (node) {
        node->content_align_x = TETHER_ALIGN_CENTER;
        node->content_align_y = TETHER_ALIGN_CENTER;
        node->padding.top = 10; node->padding.bottom = 10;
        node->padding.left = 20; node->padding.right = 20;
    }
    
    Tether_Style* s = (Tether_Style*)tether_ecs_get_component(entity, TETHER_COMPONENT_STYLE);
    if (s) {
        s->hover_color_mode = TETHER_COLOR_MODE_AUTO;
        s->press_color_mode = TETHER_COLOR_MODE_AUTO;
        s->bg_color = (Tether_Color){97, 175, 239, 255}; /* Blueish default */
    }
    
    /* Dynamically add a child Text widget to complete the Button natively */
    Tether_GUID text_entity = tether_widget_create_text(entity);
    Tether_TextStyle* ts = (Tether_TextStyle*)tether_ecs_get_component(text_entity, TETHER_COMPONENT_TEXT_STYLE);
    if (ts) {
        ts->font_size = 18.0f;
    }
    Tether_Style* ts_style = (Tether_Style*)tether_ecs_get_component(text_entity, TETHER_COMPONENT_STYLE);
    if (ts_style) {
        ts_style->bg_color = (Tether_Color){255, 255, 255, 255}; /* White text */
    }
    tether_ecs_set_text_string(text_entity, "Button");
    
    return entity;
}
