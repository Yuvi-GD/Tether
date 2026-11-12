#include "tether/ui/widgets/tether_checkbox.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"

/* Forward declaration */
static void checkbox_handle_click(Tether_GUID entity, const Tether_Event *event_info, void *user_data);
static void checkbox_update(Tether_GUID entity, bool state_changed);
static void checkbox_handle_feedback(Tether_GUID entity, const Tether_Event *event_info, void *user_data);

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

static void checkbox_update(Tether_GUID box, bool state_changed) {
    if (!tether_ui_has_bool(box)) return;
    bool checked = tether_ui_get_bool(box);

    Tether_Color border = checked ? (Tether_Color){38, 112, 232, 255} : (Tether_Color){111, 111, 118, 255};
    if (state_changed) {
        tether_ui_set_border(box, 1.5f, border, 4.0f);
        Tether_GUID indicator = tether_widget_get_child_at(box, 0);
        if (indicator != TETHER_INVALID_GUID) {
            tether_ui_set_color(indicator, checked ? (Tether_Color){38, 112, 232, 255} : (Tether_Color){0, 0, 0, 0});
        }
    }
    Tether_Shape *shape = tether_ecs_get_component(box, TETHER_COMPONENT_SHAPE);
    if (!shape) return;
    border = shape->border_color;
    int offset = tether_ui_interaction_offset(box);
    tether_ui_set_active_color(box, tether_ui_adjust_color(shape->bg_color, offset));
    tether_ui_set_active_border(box, tether_ui_adjust_color(border, offset));
}

Tether_GUID tether_ui_checkbox(Tether_GUID parent) {
    Tether_GUID box = tether_ui_panel(parent);
    tether_ui_set_container(box, TETHER_FLOW_NONE);
    tether_ui_set_size(box, 20.0f, 20.0f);
    Tether_FlexSlot *flex = tether_ecs_get_component(box, TETHER_COMPONENT_FLEX_SLOT);
    if (flex) {
        flex->align_self_x = TETHER_ALIGN_START;
        flex->align_self_y = TETHER_ALIGN_CENTER;
    }
    tether_ui_set_color(box, (Tether_Color){43, 43, 46, 255});
    tether_ui_set_border(box, 1.5f, (Tether_Color){111, 111, 118, 255}, 4.0f);
    tether_widget_add_interaction(box, TETHER_HIT_BLOCK);
    tether_ecs_add_component(box, TETHER_COMPONENT_BOOL_STATE);

    /* The root is the box; its only child is the replaceable indicator. */
    Tether_GUID indicator = tether_ui_panel(box);
    tether_ui_set_size(indicator, 15.0f, 15.0f);
    tether_ui_set_anchor(indicator, 0.5f, 0.5f, 0.5f, 0.5f);
    tether_ui_set_offsets(indicator, -7.5f, -7.5f, -7.5f, -7.5f);
    tether_ui_set_border(indicator, 0.0f, (Tether_Color){0, 0, 0, 0}, 2.0f);

    checkbox_update(box, true);

    tether_bind_event(box, TETHER_EVENT_HOVER_ENTER, checkbox_handle_feedback, NULL);
    tether_bind_event(box, TETHER_EVENT_HOVER_EXIT, checkbox_handle_feedback, NULL);
    tether_bind_event(box, TETHER_EVENT_PRESS, checkbox_handle_feedback, NULL);
    tether_bind_event(box, TETHER_EVENT_RELEASE, checkbox_handle_feedback, NULL);
    tether_bind_event(box, TETHER_EVENT_CLICK, checkbox_handle_click, NULL);

    return box;
}

/* ========================================================================== */
/* 2. Internal Event Handling                                                 */
/* ========================================================================== */

static void checkbox_handle_feedback(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    checkbox_update(entity, false);
}

static void checkbox_handle_click(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    if (!tether_ui_has_bool(entity)) return;

    tether_ui_toggle_bool(entity);
    checkbox_update(entity, true);
    tether_dispatch_event(entity, TETHER_EVENT_CHANGE);
}

/* ========================================================================== */
/* 3. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void tether_ui_set_checked(Tether_GUID checkbox, bool checked) {
    if (checkbox == TETHER_INVALID_GUID) return;
    tether_ui_set_bool(checkbox, checked);
    checkbox_update(checkbox, true);
    tether_dispatch_event(checkbox, TETHER_EVENT_CHANGE);
}

bool tether_ui_get_checked(Tether_GUID checkbox) {
    return tether_ui_get_bool(checkbox);
}
