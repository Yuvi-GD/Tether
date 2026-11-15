#include "tether/ui/widgets/tether_collapsible.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/widgets/tether_text.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"

/* Forward declaration */
static void collapsible_handle_click(TUID entity, const Tether_Event *event_info, void *user_data);
static void collapsible_handle_feedback(TUID entity, const Tether_Event *event_info, void *user_data);

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

TUID tether_ui_collapsible(TUID parent) {
    TUID outer = tether_widget_create_container(parent);
    tether_ui_set_container(outer, TETHER_FLOW_COLUMN);
    tether_ui_set_gap(outer, 0.0f, 4.0f);

    /* Attach ECS BoolState component to outer */
    Tether_BoolState *bs = (Tether_BoolState*)tether_ecs_add_component(outer, TETHER_COMPONENT_BOOL_STATE);
    if (bs) bs->value = false;

    TUID header = tether_ui_panel(outer);
    tether_ui_set_container(header, TETHER_FLOW_ROW);
    tether_ui_set_gap(header, 8.0f, 0.0f);
    tether_ui_set_size(header, 0.0f, 36.0f);
    tether_ui_set_padding(header, 6.0f, 12.0f, 6.0f, 12.0f);
    tether_ui_set_content_align(header, TETHER_ALIGN_START, TETHER_ALIGN_CENTER);
    tether_ui_set_color(header, (Tether_Color){43, 43, 46, 255});
    tether_ui_set_border(header, 1.0f, (Tether_Color){75, 75, 81, 255}, 6.0f);
    tether_widget_add_interaction(header, TETHER_HIT_BLOCK);

    TUID arrow = tether_ui_text(header);
    tether_ui_set_text(arrow, "<");
    tether_ui_set_text_size(arrow, 24.0f);
    tether_ui_set_text_color(arrow, (Tether_Color){175, 175, 175, 255});
    Tether_RenderTransform *transform = tether_ecs_get_component(arrow, TETHER_COMPONENT_RENDER_TRANSFORM);
    if (transform) {
        transform->rotation_deg = 180.0f;
        transform->render_pivot = (Tether_Vec2){0.5f, 0.5f};
    }

    TUID title_txt = tether_ui_text(header);
    tether_ui_set_text(title_txt, "Collapsible");
    tether_ui_set_text_size(title_txt, 15.0f);
    tether_ui_set_text_color(title_txt, (Tether_Color){225, 225, 225, 255});
    tether_ui_set_flex(title_txt, 1.0f);

    TUID content = tether_widget_create_container(outer);
    tether_ui_set_container(content, TETHER_FLOW_COLUMN);
    tether_ui_set_gap(content, 0.0f, 6.0f);
    tether_ui_set_padding(content, 4.0f, 4.0f, 4.0f, 4.0f);
    tether_widget_set_visibility(content, TETHER_COLLAPSED);

    tether_bind_event(header, TETHER_EVENT_CLICK, collapsible_handle_click, NULL);
    tether_bind_event(header, TETHER_EVENT_HOVER_ENTER, collapsible_handle_feedback, NULL);
    tether_bind_event(header, TETHER_EVENT_HOVER_EXIT, collapsible_handle_feedback, NULL);
    tether_bind_event(header, TETHER_EVENT_PRESS, collapsible_handle_feedback, NULL);
    tether_bind_event(header, TETHER_EVENT_RELEASE, collapsible_handle_feedback, NULL);
    return outer;
}

/* ========================================================================== */
/* 2. Internal Event Handling                                                 */
/* ========================================================================== */

static void collapsible_handle_click(TUID entity, const Tether_Event *event_info, void *user_data) {
    (void)user_data;
    if (event_info->type != TETHER_EVENT_CLICK) return;
    TUID parent = tether_widget_get_parent(entity);
    if (parent == TETHER_INVALID_TUID) return;
    tether_ui_toggle_collapsible(parent);
}

static void collapsible_handle_feedback(TUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    int adjustment = tether_ui_interaction_offset(entity);
    if (adjustment) {
        tether_ui_set_active_color(entity, tether_ui_adjust_color(tether_ui_get_color(entity), adjustment));
    } else {
        tether_ui_reset_active_color(entity);
    }
}

/* ========================================================================== */
/* 3. Operations (Setters & Getters)                                          */
/* ========================================================================== */

TUID tether_ui_collapsible_content(TUID collapsible) {
    if (collapsible == TETHER_INVALID_TUID) return TETHER_INVALID_TUID;
    return tether_widget_get_child_at(collapsible, 1);
}

void tether_ui_toggle_collapsible(TUID collapsible) {
    if (collapsible == TETHER_INVALID_TUID) return;
    TUID header = tether_widget_get_child_at(collapsible, 0);
    TUID content = tether_widget_get_child_at(collapsible, 1);
    TUID arrow = tether_widget_get_child_at(header, 0);

    bool new_open = tether_ui_toggle_bool(collapsible);

    tether_widget_set_visibility(content, new_open ? TETHER_VISIBLE : TETHER_COLLAPSED);
    if (arrow != TETHER_INVALID_TUID) {
        Tether_RenderTransform *transform = tether_ecs_get_component(arrow, TETHER_COMPONENT_RENDER_TRANSFORM);
        if (transform) {
            transform->rotation_deg = new_open ? -90.0f : 180.0f;
            tether_ecs_add_component(arrow, TETHER_COMPONENT_DIRTY_TEXT_CONTENT);
            tether_ecs_add_component(arrow, TETHER_COMPONENT_DIRTY_VISUAL);
        }
    }
    tether_dispatch_event(collapsible, TETHER_EVENT_CHANGE);
}

bool tether_ui_is_collapsible_open(TUID collapsible) {
    if (collapsible == TETHER_INVALID_TUID) return false;
    if (tether_ui_has_bool(collapsible)) {
        return tether_ui_get_bool(collapsible);
    }

    TUID content = tether_ui_collapsible_content(collapsible);
    if (content == TETHER_INVALID_TUID) return false;
    return tether_widget_is_visible(content);
}

void tether_ui_set_collapsible_title(TUID collapsible, const char *title) {
    if (collapsible == TETHER_INVALID_TUID || !title) return;
    TUID header = tether_widget_get_child_at(collapsible, 0);
    if (header == TETHER_INVALID_TUID) return;
    TUID title_txt = tether_widget_get_child_at(header, 1);
    if (title_txt != TETHER_INVALID_TUID) {
        tether_ui_set_text(title_txt, title);
    }
}
