#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_input.h"
#include <string.h>

/* ========================================================================== */
/* Primitives                                                                 */
/* ========================================================================== */

/* Create a visual panel with shape (bg_color, border, radius) */
Tether_GUID tether_ui_panel(Tether_GUID parent) {
    Tether_GUID entity = tether_widget_create_container(parent);
    Tether_Shape *shape = tether_ecs_add_component(entity, TETHER_COMPONENT_SHAPE);
    if (shape) shape->bg_color = (Tether_Color){30, 36, 48, 255};
    return entity;
}

/* ========================================================================== */
/*  Containers                                                                */
/* ========================================================================== */

/* Container Layout Configuration */
void tether_ui_set_container(Tether_GUID entity, Tether_Flow flow) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->flow = flow;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
    }
}

void tether_ui_set_position(Tether_GUID entity, Tether_PositionMode position) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->position = position;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_HIERARCHY);
    }
}

Tether_PositionMode tether_ui_get_position(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return TETHER_POSITION_RELATIVE;
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    return layout ? layout->position : TETHER_POSITION_RELATIVE;
}

void tether_ui_set_content_align(Tether_GUID entity, Tether_Align align_x, Tether_Align align_y) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->content_align_x = align_x;
        layout->content_align_y = align_y;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
    }
}

void tether_ui_set_interactable(Tether_GUID entity, Tether_HitBehavior behavior) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Interactable *interact = (Tether_Interactable*)tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (interact) {
        interact->hit_behavior = behavior;
    }
}

Tether_HitBehavior tether_ui_get_interactable(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return TETHER_HIT_IGNORE;
    Tether_Interactable *interact = (Tether_Interactable*)tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    return interact ? interact->hit_behavior : TETHER_HIT_IGNORE;
}

/* ========================================================================== */
/* Boolean State Operations                                                   */
/* ========================================================================== */

bool tether_ui_has_bool(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return false;
    return tether_ecs_get_component(entity, TETHER_COMPONENT_BOOL_STATE) != NULL;
}

bool tether_ui_get_bool(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return false;
    Tether_BoolState *b = (Tether_BoolState*)tether_ecs_get_component(entity, TETHER_COMPONENT_BOOL_STATE);
    return b ? b->value : false;
}

bool tether_ui_set_bool(Tether_GUID entity, bool value) {
    if (entity == TETHER_INVALID_GUID) return false;
    Tether_BoolState *b = (Tether_BoolState*)tether_ecs_get_component(entity, TETHER_COMPONENT_BOOL_STATE);
    if (!b) return false;
    b->value = value;
    tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    return true;
}

bool tether_ui_toggle_bool(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return false;
    Tether_BoolState *b = (Tether_BoolState*)tether_ecs_get_component(entity, TETHER_COMPONENT_BOOL_STATE);
    if (!b) return false;
    b->value = !b->value;
    tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    return b->value;
}

/* ========================================================================== */
/* Value Range Operations                                                     */
/* ========================================================================== */

float tether_ui_get_value(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return 0.0f;
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(entity, TETHER_COMPONENT_VALUE_RANGE);
    return vr ? vr->value : 0.0f;
}

bool tether_ui_set_value(Tether_GUID entity, float value) {
    if (entity == TETHER_INVALID_GUID) return false;
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(entity, TETHER_COMPONENT_VALUE_RANGE);
    if (!vr) return false;

    float clamped = value < vr->min_val ? vr->min_val : (value > vr->max_val ? vr->max_val : value);
    vr->value = clamped;
    return true;
}

void tether_ui_set_range(Tether_GUID entity, float min_val, float max_val) {
    if (entity == TETHER_INVALID_GUID || max_val <= min_val) return;
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(entity, TETHER_COMPONENT_VALUE_RANGE);
    if (!vr) return;

    vr->min_val = min_val;
    vr->max_val = max_val;
    tether_ui_set_value(entity, vr->value);
}

void tether_ui_get_range(Tether_GUID entity, float *out_min, float *out_max) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(entity, TETHER_COMPONENT_VALUE_RANGE);
    if (vr) {
        if (out_min) *out_min = vr->min_val;
        if (out_max) *out_max = vr->max_val;
    }
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
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_padding(Tether_GUID entity, float top, float right, float bottom, float left) {
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->padding = (Tether_Edges){top, right, bottom, left};
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_gap(Tether_GUID entity, float gap_x, float gap_y) {
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->gap.x = gap_x;
        layout->gap.y = gap_y;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_flex(Tether_GUID entity, float fill_ratio) {
    Tether_FlexSlot *flex = (Tether_FlexSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    if (flex) {
        flex->fill_ratio = fill_ratio;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_anchor(Tether_GUID entity, float min_x, float min_y, float max_x, float max_y) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_AnchorSlot *ta = (Tether_AnchorSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_ANCHOR_SLOT);
    if (ta) {
        ta->anchor_min = (Tether_Vec2){min_x, min_y};
        ta->anchor_max = (Tether_Vec2){max_x, max_y};
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_offsets(Tether_GUID entity, float top, float right, float bottom, float left) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_AnchorSlot *ta = (Tether_AnchorSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_ANCHOR_SLOT);
    if (ta) {
        ta->offset = (Tether_Edges){top, right, bottom, left};
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_color(Tether_GUID entity, Tether_Color bg_color) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(entity, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->bg_color = bg_color;
        shape->active_bg = bg_color;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

Tether_Color tether_ui_get_color(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return (Tether_Color){0, 0, 0, 0};
    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(entity, TETHER_COMPONENT_SHAPE);
    return shape ? shape->bg_color : (Tether_Color){0, 0, 0, 0};
}

Tether_Color tether_ui_adjust_color(Tether_Color color, int offset) {
    /* Bright colors cannot be made visibly brighter. */
    if (offset > 0 && (int)color.r + color.g + color.b > 600) offset = -offset;
    int channels[] = {color.r, color.g, color.b};
    for (int i = 0; i < 3; ++i) {
        /* Clamp before addition so even extreme offsets cannot overflow. */
        if (offset > 255 - channels[i]) channels[i] = 255;
        else if (offset < -channels[i]) channels[i] = 0;
        else channels[i] += offset;
    }
    return (Tether_Color){(uint8_t)channels[0], (uint8_t)channels[1],
                          (uint8_t)channels[2], color.a};
}

int tether_ui_interaction_offset(Tether_GUID entity) {
    Tether_Interactable *interaction =
        tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (!interaction || !interaction->is_enabled) return 0;
    if (interaction->is_pressed) return -40;
    if (interaction->is_hovered || interaction->has_focus) return 20;
    return 0;
}

void tether_ui_set_border(Tether_GUID entity, float width, Tether_Color color, float radius) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(entity, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->border_width = width;
        shape->border_color = color;
        shape->active_border = color;
        shape->border_radius = (Tether_Edges){radius, radius, radius, radius};
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_active_color(Tether_GUID entity, Tether_Color color) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(entity, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->active_bg = color;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_reset_active_color(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(entity, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->active_bg = shape->bg_color;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_active_border(Tether_GUID entity, Tether_Color color) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(entity, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->active_border = color;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_reset_active_border(Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(entity, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->active_border = shape->border_color;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}
