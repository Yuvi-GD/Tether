#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include <string.h>

static Tether_GUID resolve_text_entity(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return TETHER_INVALID_GUID;
    if (tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT) != NULL) {
        return entity;
    }
    Tether_Hierarchy *h = (Tether_Hierarchy*)tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    if (h && h->first_child != TETHER_INVALID_GUID) {
        Tether_GUID child = h->first_child;
        while (child != TETHER_INVALID_GUID) {
            if (tether_ecs_get_component(child, TETHER_COMPONENT_TEXT) != NULL) {
                return child;
            }
            Tether_Hierarchy *ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
            child = ch ? ch->next_sibling : TETHER_INVALID_GUID;
        }
    }
    return TETHER_INVALID_GUID;
}

/* ========================================================================== */
/* Containers & Flex Layout                                                   */
/* ========================================================================== */

Tether_GUID tether_ui_panel(Tether_GUID parent, Tether_Color bg_color, float radius) {
    Tether_GUID panel = tether_widget_create_panel(parent);
    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(panel, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->bg_color = bg_color;
        shape->border_radius = (Tether_Edges){radius, radius, radius, radius};
    }
    return panel;
}

Tether_GUID tether_ui_row(Tether_GUID parent, float gap_x) {
    Tether_GUID row = tether_widget_create_panel(parent);
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(row, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->flow = TETHER_FLOW_ROW;
        layout->gap.x = gap_x;
        layout->gap.y = 0.0f;
    }
    return row;
}

Tether_GUID tether_ui_column(Tether_GUID parent, float gap_y) {
    Tether_GUID col = tether_widget_create_panel(parent);
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(col, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->flow = TETHER_FLOW_COLUMN;
        layout->gap.x = 0.0f;
        layout->gap.y = gap_y;
    }
    return col;
}

/* ========================================================================== */
/* Content (Unified Text)                                                     */
/* ========================================================================== */

Tether_GUID tether_ui_text(Tether_GUID parent, const char *string, float font_size, Tether_Color color) {
    Tether_GUID entity = tether_widget_create_text(parent);
    Tether_Text *text = (Tether_Text*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT);
    if (text) {
        text->font_size = font_size > 0.0f ? font_size : 16.0f;
        text->color = color;
    }
    if (string) {
        tether_ui_set_text(entity, string);
    }
    return entity;
}

bool tether_ui_set_text(Tether_GUID entity, const char *string) {
    Tether_GUID target = resolve_text_entity(entity);
    if (target == TETHER_INVALID_GUID) return false;

    if (tether_ecs_set_text_string(target, string ? string : "")) {
        tether_ecs_add_component(target, TETHER_COMPONENT_DIRTY_LAYOUT);
        tether_ecs_add_component(target, TETHER_COMPONENT_DIRTY_VISUAL);
        return true;
    }
    return false;
}

const char* tether_ui_get_text(Tether_GUID entity) {
    Tether_GUID target = resolve_text_entity(entity);
    if (target == TETHER_INVALID_GUID) return NULL;
    return tether_ecs_get_text_string(target);
}

/* ========================================================================== */
/* Interactive Controls                                                       */
/* ========================================================================== */

Tether_GUID tether_ui_button(Tether_GUID parent, const char *caption, float font_size,
                             Tether_Color bg_color, Tether_EventCallback callback, void *user_data) {
    Tether_GUID btn = tether_widget_create_panel(parent);

    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(btn, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->size_box.y = 44.0f;
        layout->padding = (Tether_Edges){8.0f, 16.0f, 8.0f, 16.0f};
        layout->content_align_x = TETHER_ALIGN_CENTER;
        layout->content_align_y = TETHER_ALIGN_CENTER;
    }

    Tether_FlexSlot *flex = (Tether_FlexSlot*)tether_ecs_get_component(btn, TETHER_COMPONENT_FLEX_SLOT);
    if (flex) {
        flex->fill_ratio = 1.0f;
    }

    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(btn, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->bg_color = bg_color;
        shape->border_radius = (Tether_Edges){7.0f, 7.0f, 7.0f, 7.0f};
    }

    Tether_Interactable *interact = (Tether_Interactable*)tether_ecs_get_component(btn, TETHER_COMPONENT_INTERACTABLE);
    if (interact) {
        interact->hit_behavior = TETHER_HIT_BLOCK;
        interact->hover_color_mode = TETHER_COLOR_MODE_AUTO;
        interact->press_color_mode = TETHER_COLOR_MODE_AUTO;
    }

    Tether_GUID label = tether_ui_text(btn, caption, font_size > 0.0f ? font_size : 16.0f, (Tether_Color){222, 232, 247, 255});
    Tether_Text *text = (Tether_Text*)tether_ecs_get_component(label, TETHER_COMPONENT_TEXT);
    if (text) {
        text->align_x = TETHER_ALIGN_CENTER;
        text->align_y = TETHER_ALIGN_CENTER;
    }
    Tether_FlexSlot *label_flex = (Tether_FlexSlot*)tether_ecs_get_component(label, TETHER_COMPONENT_FLEX_SLOT);
    if (label_flex) {
        label_flex->fill_ratio = 1.0f;
    }

    if (callback) {
        tether_bind_event(btn, TETHER_EVENT_CLICK, callback, user_data);
    }

    return btn;
}

/* ========================================================================== */
/* Fluent Layout & Style Modifiers                                            */
/* ========================================================================== */

void tether_ui_set_size(Tether_GUID entity, float width, float height) {
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->size_box.x = width;
        layout->size_box.y = height;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
    }
}

void tether_ui_set_padding(Tether_GUID entity, float top, float right, float bottom, float left) {
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->padding = (Tether_Edges){top, right, bottom, left};
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
    }
}

void tether_ui_set_gap(Tether_GUID entity, float gap_x, float gap_y) {
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->gap.x = gap_x;
        layout->gap.y = gap_y;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
    }
}

void tether_ui_set_flex(Tether_GUID entity, float fill_ratio) {
    Tether_FlexSlot *flex = (Tether_FlexSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    if (flex) {
        flex->fill_ratio = fill_ratio;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
    }
}

void tether_ui_set_color(Tether_GUID entity, Tether_Color bg_color) {
    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(entity, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->bg_color = bg_color;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_border(Tether_GUID entity, float width, Tether_Color color, float radius) {
    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(entity, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->border_width = width;
        shape->border_color = color;
        shape->border_radius = (Tether_Edges){radius, radius, radius, radius};
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_text_color(Tether_GUID entity, Tether_Color color) {
    Tether_GUID target = resolve_text_entity(entity);
    if (target != TETHER_INVALID_GUID) {
        Tether_Text *text = (Tether_Text*)tether_ecs_get_component(target, TETHER_COMPONENT_TEXT);
        if (text) {
            text->color = color;
            tether_ecs_add_component(target, TETHER_COMPONENT_DIRTY_VISUAL);
        }
    }
}

void tether_ui_set_text_align(Tether_GUID entity, Tether_Align align_x, Tether_Align align_y) {
    Tether_GUID target = resolve_text_entity(entity);
    if (target != TETHER_INVALID_GUID) {
        Tether_Text *text = (Tether_Text*)tether_ecs_get_component(target, TETHER_COMPONENT_TEXT);
        if (text) {
            text->align_x = align_x;
            text->align_y = align_y;
            tether_ecs_add_component(target, TETHER_COMPONENT_DIRTY_LAYOUT);
            tether_ecs_add_component(target, TETHER_COMPONENT_DIRTY_VISUAL);
        }
    }
}
