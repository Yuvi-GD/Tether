#include "tether/ui/widgets/tether_slider.h"
#include "tether/ui/widgets/tether_progress_bar.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_input.h"
#include <math.h>

#define SLIDER_THUMB_DIAMETER 20.0f
#define SLIDER_THUMB_RADIUS   (SLIDER_THUMB_DIAMETER * 0.5f)

#define SLIDER_HEIGHT 8.0f
#define SLIDER_RADIUS 2.0f

/* Forward declarations */
static void slider_update(Tether_GUID slider);
static void slider_handle_interaction(Tether_GUID entity, const Tether_Event *event_info, void *user_data);

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

static void slider_update(Tether_GUID slider) {
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(slider, TETHER_COMPONENT_VALUE_RANGE);
    if (!vr) return;

    float range = vr->max_val - vr->min_val;
    float frac = (range > 0.0f) ? (vr->value - vr->min_val) / range : 0.0f;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    Tether_GUID fill = tether_widget_get_child_at(slider, 0);
    if (fill != TETHER_INVALID_GUID) {
        float fill_offset_right = (frac <= 0.0f) ? -SLIDER_THUMB_RADIUS : (frac >= 1.0f ? 0.0f : -(0.5f - frac) * SLIDER_THUMB_DIAMETER);
        tether_ui_set_anchor(fill, 0.0f, 0.0f, frac, 1.0f);
        tether_ui_set_offsets(fill, 0.0f, fill_offset_right, 0.0f, 0.0f);
        tether_ecs_add_component(fill, TETHER_COMPONENT_DIRTY_VISUAL);
    }

    Tether_GUID thumb = tether_widget_get_child_at(slider, 1);
    if (thumb != TETHER_INVALID_GUID) {
        tether_ui_set_anchor(thumb, frac, 0.5f, frac, 0.5f);
        tether_ui_set_offsets(thumb, -SLIDER_THUMB_RADIUS,
                                     -(1.0f - frac) * SLIDER_THUMB_DIAMETER,
                                     -SLIDER_THUMB_RADIUS,
                                     -frac * SLIDER_THUMB_DIAMETER);
        tether_ecs_add_component(thumb, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

Tether_GUID tether_ui_slider(Tether_GUID parent) {
    /* Slider is directly a flexible progress bar with an interactive thumb */
    Tether_GUID slider = tether_ui_progress_bar(parent);

    tether_ui_set_size(slider, 0.0f, SLIDER_HEIGHT);
    tether_ui_set_color(slider, (Tether_Color){54, 54, 54, 255});
    tether_ui_set_border(slider, 0.0f, (Tether_Color){0, 0, 0, 0}, SLIDER_RADIUS);

    Tether_FlexSlot *flex = (Tether_FlexSlot*)tether_ecs_get_component(slider, TETHER_COMPONENT_FLEX_SLOT);
    if (flex) {
        flex->align_self_x = TETHER_ALIGN_FILL;
        flex->fill_ratio = 0.0f;
    }

    Tether_GUID fill = tether_widget_get_child_at(slider, 0);
    if (fill != TETHER_INVALID_GUID) {
        tether_ui_set_border(fill, 0.0f, (Tether_Color){0, 0, 0, 0}, SLIDER_RADIUS);
    }

    /* Allow the thumb to visually overflow the progress bar track */
    Tether_Overflow *of = (Tether_Overflow*)tether_ecs_get_component(slider, TETHER_COMPONENT_OVERFLOW);
    if (of) {
        of->x = TETHER_OVERFLOW_VISIBLE;
        of->y = TETHER_OVERFLOW_VISIBLE;
    }

    tether_widget_add_interaction(slider, TETHER_HIT_BLOCK);

    /* Default slider range (0 to 100) */
    tether_ui_set_range(slider, 0.0f, 100.0f);
    tether_ui_set_value(slider, 0.0f);

    /* Child 1: Thumb: 20x20 circle */
    Tether_GUID thumb = tether_ui_panel(slider);
    tether_ui_set_size(thumb, SLIDER_THUMB_DIAMETER, SLIDER_THUMB_DIAMETER);
    tether_ui_set_anchor(thumb, 0.0f, 0.5f, 0.0f, 0.5f);
    tether_ui_set_offsets(thumb, -SLIDER_THUMB_RADIUS, -SLIDER_THUMB_DIAMETER, -SLIDER_THUMB_RADIUS, 0.0f);
    tether_ui_set_color(thumb, (Tether_Color){155, 155, 155, 255});
    tether_ui_set_border(thumb, 1.5f, (Tether_Color){145, 145, 145, 255}, SLIDER_THUMB_RADIUS);

    slider_update(slider);

    /* Bind internal visual feedback and input events */
    tether_bind_event(slider, TETHER_EVENT_HOVER_ENTER, slider_handle_interaction, NULL);
    tether_bind_event(slider, TETHER_EVENT_HOVER_EXIT, slider_handle_interaction, NULL);
    tether_bind_event(slider, TETHER_EVENT_PRESS, slider_handle_interaction, NULL);
    tether_bind_event(slider, TETHER_EVENT_RELEASE, slider_handle_interaction, NULL);
    tether_bind_event(slider, TETHER_EVENT_DRAG, slider_handle_interaction, NULL);
    tether_bind_event(slider, TETHER_EVENT_CLICK, slider_handle_interaction, NULL);

    return slider;
}

/* ========================================================================== */
/* 2. Internal Event Handling                                                 */
/* ========================================================================== */

static void slider_update_feedback(Tether_GUID slider, Tether_EventType type) {
    Tether_GUID thumb = tether_widget_get_child_at(slider, 1);
    if (thumb == TETHER_INVALID_GUID) return;

    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(thumb, TETHER_COMPONENT_SHAPE);
    if (!shape) return;

    (void)type;
    int offset = tether_ui_interaction_offset(slider);
    shape->active_bg = tether_ui_adjust_color(shape->bg_color, offset);
    shape->active_border = tether_ui_adjust_color(shape->border_color, offset);
    tether_ecs_add_component(thumb, TETHER_COMPONENT_DIRTY_VISUAL);
}

/* Keep pointer and programmatic changes on the same configured step. */
static float slider_snap_value(Tether_GUID slider, float value) {
    Tether_ValueRange *range = tether_ecs_get_component(slider, TETHER_COMPONENT_VALUE_RANGE);
    if (!range || !isfinite(value) || !isfinite(range->step) || range->step <= 0.0f) return value;
    double steps = ((double)value - range->min_val) / range->step;
    return (float)(range->min_val + round(steps) * range->step);
}

static void slider_handle_input(Tether_GUID entity) {
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(entity, TETHER_COMPONENT_VALUE_RANGE);
    if (!vr) return;

    float mx = 0.0f, my = 0.0f;
    tether_input_get_pointer_pos(&mx, &my);

    Tether_SlotTransform *st = (Tether_SlotTransform*)tether_ecs_get_component(entity, TETHER_COMPONENT_SLOT_TRANSFORM);
    if (!st || st->width <= SLIDER_THUMB_DIAMETER) return;

    float usable_w = st->width - SLIDER_THUMB_DIAMETER;
    float frac = (mx - st->x - SLIDER_THUMB_RADIUS) / usable_w;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    float new_val = vr->min_val + frac * (vr->max_val - vr->min_val);
    float old_val = tether_ui_get_value(entity);
    tether_ui_set_value(entity, slider_snap_value(entity, new_val));
    if (tether_ui_get_value(entity) == old_val) return;
    slider_update(entity);
    tether_dispatch_event(entity, TETHER_EVENT_CHANGE);
}

static void slider_handle_interaction(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)user_data;
    if (!tether_widget_is_enabled(entity)) return;
    Tether_Interactable *interaction = tether_ecs_get_component(entity, TETHER_COMPONENT_INTERACTABLE);
    if (event_info->type == TETHER_EVENT_DRAG && (!interaction || !interaction->is_pressed)) return;
    slider_update_feedback(entity, event_info->type);
    /* Press already sets the value; the following click must not set it twice. */
    if (event_info->type == TETHER_EVENT_PRESS || event_info->type == TETHER_EVENT_DRAG) {
        slider_handle_input(entity);
    }
}

/* ========================================================================== */
/* 3. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void tether_ui_set_slider_value(Tether_GUID slider, float value) {
    if (slider == TETHER_INVALID_GUID) return;
    tether_ui_set_value(slider, slider_snap_value(slider, value));
    slider_update(slider);
    tether_dispatch_event(slider, TETHER_EVENT_CHANGE);
}

float tether_ui_get_slider_value(Tether_GUID slider) {
    return tether_ui_get_value(slider);
}

void tether_ui_set_slider_range(Tether_GUID slider, float min_val, float max_val) {
    if (slider == TETHER_INVALID_GUID) return;
    tether_ui_set_range(slider, min_val, max_val);
    slider_update(slider);
}
