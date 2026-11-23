#include "tether/ui/widgets/tether_progress_bar.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_input.h"
#include <math.h>

#define BAR_HEIGHT 16.0f
#define BAR_RADIUS 4.0f

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

void tether_ui_update_progress(TUID progress_bar) {
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(progress_bar, TETHER_COMPONENT_VALUE_RANGE);
    if (!vr) return;

    float range = vr->max_val - vr->min_val;
    float frac = (range > 0.0f) ? (vr->value - vr->min_val) / range : 0.0f;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    TUID fill = tether_widget_get_child_at(progress_bar, 0);
    if (fill != TETHER_INVALID_TUID) {
        tether_ui_set_anchor(fill, 0.0f, 0.0f, frac, 1.0f);
        tether_ui_set_offsets(fill, 0.0f, 0.0f, 0.0f, 0.0f);
        tether_ecs_add_component(fill, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

static float progress_snap_value(TUID entity, float value) {
    Tether_ValueRange *range = tether_ecs_get_component(entity, TETHER_COMPONENT_VALUE_RANGE);
    if (!range || !isfinite(value) || !isfinite(range->step) || range->step <= 0.0f) return value;
    double steps = ((double)value - range->min_val) / range->step;
    return (float)(range->min_val + round(steps) * range->step);
}

static bool progress_set_pointer(TUID entity) {
    Tether_ValueRange *range = tether_ecs_get_component(entity, TETHER_COMPONENT_VALUE_RANGE);
    Tether_SlotTransform *slot = tether_ecs_get_component(entity, TETHER_COMPONENT_SLOT_TRANSFORM);
    Tether_Layout *layout = tether_ecs_get_component(entity, TETHER_COMPONENT_LAYOUT);
    if (!range || !slot || !layout) return false;
    float width = slot->width - layout->padding.left - layout->padding.right;
    if (width <= 0.0f) return false;
    float x, y;
    tether_input_get_pointer_pos(&x, &y);
    (void)y;
    float fraction = (x - slot->x - layout->padding.left) / width;
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    float value = range->min_val + fraction * (range->max_val - range->min_val);
    float previous = range->value;
    if (!tether_ui_set_value(entity, progress_snap_value(entity, value))) return false;
    return tether_ui_get_value(entity) != previous;
}

static void progress_handle_input(TUID entity, const Tether_Event *event, void *user_data) {
    (void)user_data;
    if (!tether_widget_is_enabled(entity)) return;
    Tether_Interactable *interaction = tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (event->type == TETHER_EVENT_DRAG && (!interaction || !interaction->is_pressed)) return;
    if (progress_set_pointer(entity)) {
        tether_ui_update_progress(entity);
        tether_dispatch_event(entity, TETHER_EVENT_CHANGE);
    }
}

TUID tether_ui_progress_bar(TUID parent) {
    TUID track = tether_ui_panel(parent);
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(track, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->flow = TETHER_FLOW_NONE;
        layout->size_box.x = 0.0f;
        layout->size_box.y = BAR_HEIGHT;
    }

    Tether_FlexSlot *flex = (Tether_FlexSlot*)tether_ecs_get_component(track, TETHER_COMPONENT_FLEX_SLOT);
    if (flex) {
        flex->align_self_x = TETHER_ALIGN_FILL;
        flex->fill_ratio = 0.0f;
    }

    tether_ui_set_color(track, (Tether_Color){53, 53, 58, 255});
    tether_ui_set_border(track, 0.0f, (Tether_Color){0, 0, 0, 0}, BAR_RADIUS);

    tether_ecs_add_component(track, TETHER_COMPONENT_VALUE_RANGE);

    TUID fill = tether_ui_panel(track);
    tether_ui_set_anchor(fill, 0.0f, 0.0f, 0.0f, 1.0f);
    tether_ui_set_offsets(fill, 0.0f, 0.0f, 0.0f, 0.0f);
    tether_ui_set_color(fill, (Tether_Color){38, 112, 232, 255});
    tether_ui_set_border(fill, 0.0f, (Tether_Color){0, 0, 0, 0}, BAR_RADIUS);

    tether_widget_add_interaction(track, TETHER_HIT_BLOCK);
    tether_bind_event(track, TETHER_EVENT_PRESS, progress_handle_input, NULL);
    tether_bind_event(track, TETHER_EVENT_DRAG, progress_handle_input, NULL);

    return track;
}

/* ========================================================================== */
/* 2. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void tether_ui_set_progress(TUID progress_bar, float progress) {
    if (progress_bar == TETHER_INVALID_TUID) return;
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(progress_bar, TETHER_COMPONENT_VALUE_RANGE);
    if (!vr) return;

    float clamped_prog = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
    float val = vr->min_val + clamped_prog * (vr->max_val - vr->min_val);
    tether_ui_set_value(progress_bar, val);
    tether_ui_update_progress(progress_bar);
}

void tether_ui_set_progress_value(TUID progress_bar, float value) {
    if (tether_ui_set_value(progress_bar, progress_snap_value(progress_bar, value))) {
        tether_ui_update_progress(progress_bar);
    }
}

float tether_ui_get_progress(TUID progress_bar) {
    if (progress_bar == TETHER_INVALID_TUID) return 0.0f;
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(progress_bar, TETHER_COMPONENT_VALUE_RANGE);
    if (!vr) return 0.0f;
    float range = vr->max_val - vr->min_val;
    return (range > 0.0f) ? (vr->value - vr->min_val) / range : 0.0f;
}
