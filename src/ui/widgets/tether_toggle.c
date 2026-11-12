#include "tether/ui/widgets/tether_toggle.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"

/* Forward declaration */
static void toggle_handle_click(Tether_GUID entity, const Tether_Event *event_info, void *user_data);
static void toggle_handle_feedback(Tether_GUID entity, const Tether_Event *event_info, void *user_data);

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

static void toggle_update(Tether_GUID entity, bool state_changed) {
    if (!tether_ui_has_bool(entity)) return;
    bool is_on = tether_ui_get_bool(entity);

    if (state_changed) {
        tether_ui_set_color(entity, is_on ? (Tether_Color){45, 109, 190, 255} : (Tether_Color){54, 54, 54, 255});
    }
    Tether_Color base = tether_ui_get_color(entity);
    int offset = tether_ui_interaction_offset(entity);
    tether_ui_set_active_color(entity, tether_ui_adjust_color(base, offset));

    if (!state_changed) return;
    Tether_GUID thumb = tether_widget_get_child_at(entity, 0);
    if (thumb != TETHER_INVALID_GUID) {
        if (is_on) {
            /* 46px wide container - 20px thumb - 3px margin = 23px from left */
            tether_ui_set_anchor(thumb, 1.0f, 0.5f, 1.0f, 0.5f);
            tether_ui_set_offsets(thumb, -10.0f, 3.0f, -10.0f, -23.0f);
        } else {
            tether_ui_set_anchor(thumb, 0.0f, 0.5f, 0.0f, 0.5f);
            tether_ui_set_offsets(thumb, -10.0f, -23.0f, -10.0f, 3.0f);
        }
    }
}

Tether_GUID tether_ui_toggle(Tether_GUID parent) {
    Tether_GUID entity = tether_ui_panel(parent);
    tether_ui_set_container(entity, TETHER_FLOW_NONE);
    tether_ui_set_size(entity, 46.0f, 26.0f);
    Tether_FlexSlot *flex = tether_ecs_get_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    if (flex) {
        flex->align_self_x = TETHER_ALIGN_START;
        flex->align_self_y = TETHER_ALIGN_CENTER;
    }
    tether_ui_set_border(entity, 0.0f, (Tether_Color){0, 0, 0, 0}, 13.0f);
    tether_widget_add_interaction(entity, TETHER_HIT_BLOCK);

    /* Attach ECS BoolState component */
    tether_ecs_add_component(entity, TETHER_COMPONENT_BOOL_STATE);

    /* Child 0: Thumb panel */
    Tether_GUID thumb = tether_ui_panel(entity);
    tether_ui_set_size(thumb, 20.0f, 20.0f);
    tether_ui_set_color(thumb, (Tether_Color){225, 225, 225, 255});
    tether_ui_set_border(thumb, 0.0f, (Tether_Color){0, 0, 0, 0}, 10.0f);

    toggle_update(entity, true);

    tether_bind_event(entity, TETHER_EVENT_HOVER_ENTER, toggle_handle_feedback, NULL);
    tether_bind_event(entity, TETHER_EVENT_HOVER_EXIT, toggle_handle_feedback, NULL);
    tether_bind_event(entity, TETHER_EVENT_PRESS, toggle_handle_feedback, NULL);
    tether_bind_event(entity, TETHER_EVENT_RELEASE, toggle_handle_feedback, NULL);
    /* Bind internal click logic */
    tether_bind_event(entity, TETHER_EVENT_CLICK, toggle_handle_click, NULL);

    return entity;
}

/* ========================================================================== */
/* 2. Internal Event Handling                                                 */
/* ========================================================================== */

static void toggle_handle_feedback(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    toggle_update(entity, false);
}

static void toggle_handle_click(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    if (!tether_ui_has_bool(entity)) return;

    tether_ui_toggle_bool(entity);
    toggle_update(entity, true);
    tether_dispatch_event(entity, TETHER_EVENT_CHANGE);
}

/* ========================================================================== */
/* 3. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void tether_ui_set_toggle(Tether_GUID toggle, bool is_on) {
    if (toggle == TETHER_INVALID_GUID) return;
    tether_ui_set_bool(toggle, is_on);
    toggle_update(toggle, true);
    tether_dispatch_event(toggle, TETHER_EVENT_CHANGE);
}

bool tether_ui_get_toggle(Tether_GUID toggle) {
    return tether_ui_get_bool(toggle);
}
