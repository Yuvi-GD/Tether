#include "tether/ui/widgets/tether_radio.h"
#include "tether/ui/tether_list.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/widgets/tether_text.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"

/* Forward declaration */
static void radio_handle_click(Tether_GUID entity, const Tether_Event *event_info, void *user_data);
static void radio_handle_feedback(Tether_GUID entity, const Tether_Event *event_info, void *user_data);

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

static void radio_update(Tether_GUID radio) {
    int selected = tether_list_get_selected(radio);
    int count = tether_list_get_count(radio);

    for (int i = 0; i < count; i++) {
        Tether_GUID row = tether_list_get_item(radio, i);
        if (row == TETHER_INVALID_GUID) break;
        bool is_selected = (i == selected);

        Tether_GUID ring = tether_widget_get_child_at(row, 0);
        if (ring != TETHER_INVALID_GUID) {
            tether_ui_set_border(ring, 1.5f, is_selected ? (Tether_Color){38, 112, 232, 255} : (Tether_Color){111, 111, 118, 255}, 10.0f);

            Tether_GUID dot = tether_widget_get_child_at(ring, 0);
            if (dot != TETHER_INVALID_GUID) {
                tether_ui_set_color(dot, is_selected ? (Tether_Color){38, 112, 232, 255} : (Tether_Color){0, 0, 0, 0});
            }
            radio_handle_feedback(row, &(Tether_Event){ .type = TETHER_EVENT_CHANGE }, NULL);
        }
    }
}

Tether_GUID tether_ui_radio(Tether_GUID parent) {
    Tether_GUID radio = tether_list_create(parent);

    /* Layout: default column flow for radio items */
    tether_ui_set_container(radio, TETHER_FLOW_COLUMN);
    tether_ui_set_gap(radio, 0.0f, 8.0f);

    return radio;
}

void tether_ui_set_radio_flow(Tether_GUID radio, Tether_Flow flow) {
    if (flow != TETHER_FLOW_ROW && flow != TETHER_FLOW_COLUMN) return;
    if (!tether_ecs_get_component(radio, TETHER_COMPONENT_SELECTION_STATE)) return;
    tether_ui_set_container(radio, flow);
    tether_ui_set_gap(radio, flow == TETHER_FLOW_ROW ? 8.0f : 0.0f,
                      flow == TETHER_FLOW_COLUMN ? 8.0f : 0.0f);
}

/* ========================================================================== */
/* 2. Internal Event Handling                                                 */
/* ========================================================================== */

static void radio_handle_feedback(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    Tether_GUID ring = tether_widget_get_child_at(entity, 0);
    if (ring == TETHER_INVALID_GUID) return;

    int offset = tether_ui_interaction_offset(entity);
    if (offset == 0) {
        tether_ui_reset_active_color(ring);
        tether_ui_reset_active_border(ring);
        return;
    }

    Tether_Shape *shape = tether_ecs_get_component(ring, TETHER_COMPONENT_SHAPE);
    if (!shape) return;
    Tether_Color bg = shape->bg_color;
    Tether_Color border = shape->border_color;
    tether_ui_set_active_color(ring, tether_ui_adjust_color(bg, offset));
    tether_ui_set_active_border(ring, tether_ui_adjust_color(border, offset));
}

static void radio_handle_click(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    Tether_GUID radio = tether_widget_get_parent(entity);
    if (radio == TETHER_INVALID_GUID) return;

    int count = tether_list_get_count(radio);
    for (int i = 0; i < count; i++) {
        if (tether_list_get_item(radio, i) == entity) {
            tether_ui_set_radio_selected(radio, i);
            break;
        }
    }
}

/* ========================================================================== */
/* 3. Operations (Item Management & Selection)                                */
/* ========================================================================== */

Tether_GUID tether_ui_add_radio_option(Tether_GUID radio, const char *label) {
    if (radio == TETHER_INVALID_GUID) return TETHER_INVALID_GUID;

    Tether_GUID row = tether_list_add_item(radio);
    tether_widget_add_interaction(row, TETHER_HIT_BLOCK);

    tether_ui_set_container(row, TETHER_FLOW_ROW);
    tether_ui_set_gap(row, 8.0f, 0.0f);
    tether_ui_set_content_align(row, TETHER_ALIGN_START, TETHER_ALIGN_CENTER);
    tether_ui_set_size(row, 0.0f, 20.0f);
    tether_ui_set_flex(row, 1.0f);

    /* Child 0: Outer Ring 20x20 circle, border 1.5px, radius 10px */
    Tether_GUID ring = tether_ui_panel(row);
    tether_ui_set_container(ring, TETHER_FLOW_ROW);
    tether_ui_set_content_align(ring, TETHER_ALIGN_CENTER, TETHER_ALIGN_CENTER);
    tether_ui_set_size(ring, 20.0f, 20.0f);
    tether_ui_set_color(ring, (Tether_Color){43, 43, 46, 255});
    tether_ui_set_border(ring, 1.5f, (Tether_Color){111, 111, 118, 255}, 10.0f);

    /* Child 0 of Ring: Inner Bullet Dot 10x10 circle, radius 5px, centered */
    Tether_GUID dot = tether_ui_panel(ring);
    tether_ui_set_size(dot, 10.0f, 10.0f);
    tether_ui_set_border(dot, 0.0f, (Tether_Color){0, 0, 0, 0}, 5.0f);
    tether_ui_set_color(dot, (Tether_Color){0, 0, 0, 0});

    /* Child 1: Label Text */
    Tether_GUID txt = tether_ui_text(row);
    tether_ui_set_text(txt, label ? label : "");
    tether_ui_set_text_size(txt, 15.0f);
    tether_ui_set_text_color(txt, (Tether_Color){225, 225, 225, 255});

    tether_bind_event(row, TETHER_EVENT_CLICK, radio_handle_click, NULL);
    tether_bind_event(row, TETHER_EVENT_HOVER_ENTER, radio_handle_feedback, NULL);
    tether_bind_event(row, TETHER_EVENT_HOVER_EXIT, radio_handle_feedback, NULL);
    tether_bind_event(row, TETHER_EVENT_PRESS, radio_handle_feedback, NULL);
    tether_bind_event(row, TETHER_EVENT_RELEASE, radio_handle_feedback, NULL);

    radio_update(radio);
    return row;
}

void tether_ui_set_radio_option(Tether_GUID radio, int index, const char *label) {
    Tether_GUID row = tether_list_get_item(radio, index);
    if (row == TETHER_INVALID_GUID) return;
    Tether_GUID txt = tether_widget_get_child_at(row, 1);
    if (txt != TETHER_INVALID_GUID && label) {
        tether_ui_set_text(txt, label);
    }
}

const char* tether_ui_get_radio_option(Tether_GUID radio, int index) {
    Tether_GUID row = tether_list_get_item(radio, index);
    if (row == TETHER_INVALID_GUID) return NULL;
    Tether_GUID txt = tether_widget_get_child_at(row, 1);
    return txt != TETHER_INVALID_GUID ? tether_ui_get_text(txt) : NULL;
}

void tether_ui_remove_radio_option(Tether_GUID radio, int index) {
    tether_list_remove_item(radio, index);
    radio_update(radio);
}

void tether_ui_clear_radio_options(Tether_GUID radio) {
    tether_list_clear(radio);
    radio_update(radio);
}

void tether_ui_set_radio_selected(Tether_GUID radio, int selected_index) {
    tether_list_set_selected(radio, selected_index);
    radio_update(radio);
    tether_dispatch_event(radio, TETHER_EVENT_CHANGE);
}

int tether_ui_get_radio_selected(Tether_GUID radio) {
    return tether_list_get_selected(radio);
}

void tether_ui_unselect_radio(Tether_GUID radio) {
    tether_list_unselect(radio);
    radio_update(radio);
}
