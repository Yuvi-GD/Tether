#include "tether/ui/widgets/tether_progress_bar.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"

#define BAR_HEIGHT 16.0f
#define BAR_RADIUS 4.0f

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

static void progress_bar_sync_visuals(Tether_GUID progress_bar) {
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(progress_bar, TETHER_COMPONENT_VALUE_RANGE);
    if (!vr) return;

    float range = vr->max_val - vr->min_val;
    float frac = (range > 0.0f) ? (vr->value - vr->min_val) / range : 0.0f;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    Tether_GUID fill = tether_widget_get_child_at(progress_bar, 0);
    if (fill != TETHER_INVALID_GUID) {
        tether_ui_set_anchor(fill, 0.0f, 0.0f, frac, 1.0f);
        tether_ui_set_offsets(fill, 0.0f, 0.0f, 0.0f, 0.0f);
        tether_ecs_add_component(fill, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

Tether_GUID tether_ui_progress_bar(Tether_GUID parent) {
    Tether_GUID track = tether_ui_panel(parent);
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

    Tether_GUID fill = tether_ui_panel(track);
    tether_ui_set_anchor(fill, 0.0f, 0.0f, 0.0f, 1.0f);
    tether_ui_set_offsets(fill, 0.0f, 0.0f, 0.0f, 0.0f);
    tether_ui_set_color(fill, (Tether_Color){38, 112, 232, 255});
    tether_ui_set_border(fill, 0.0f, (Tether_Color){0, 0, 0, 0}, BAR_RADIUS);

    return track;
}

/* ========================================================================== */
/* 2. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void tether_ui_set_progress(Tether_GUID progress_bar, float progress) {
    if (progress_bar == TETHER_INVALID_GUID) return;
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(progress_bar, TETHER_COMPONENT_VALUE_RANGE);
    if (!vr) return;

    float clamped_prog = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
    float val = vr->min_val + clamped_prog * (vr->max_val - vr->min_val);
    tether_ui_set_value(progress_bar, val);
    progress_bar_sync_visuals(progress_bar);
}

float tether_ui_get_progress(Tether_GUID progress_bar) {
    if (progress_bar == TETHER_INVALID_GUID) return 0.0f;
    Tether_ValueRange *vr = (Tether_ValueRange*)tether_ecs_get_component(progress_bar, TETHER_COMPONENT_VALUE_RANGE);
    if (!vr) return 0.0f;
    float range = vr->max_val - vr->min_val;
    return (range > 0.0f) ? (vr->value - vr->min_val) / range : 0.0f;
}
