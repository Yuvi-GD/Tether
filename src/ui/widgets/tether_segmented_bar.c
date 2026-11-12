#include "tether/ui/widgets/tether_segmented_bar.h"
#include "tether/ui/tether_list.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/widgets/tether_text.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"

/* Forward declaration */
static void segment_handle_click(Tether_GUID entity, const Tether_Event *event_info, void *user_data);
static void segment_handle_feedback(Tether_GUID entity, const Tether_Event *event_info, void *user_data);

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

static void segmented_bar_update(Tether_GUID bar) {
    int selected = tether_list_get_selected(bar);
    int count = tether_list_get_count(bar);

    for (int i = 0; i < count; i++) {
        Tether_GUID seg = tether_list_get_item(bar, i);
        if (seg == TETHER_INVALID_GUID) break;
        bool is_selected = (i == selected);

        tether_ui_set_color(seg, is_selected ? (Tether_Color){45, 109, 190, 255} : (Tether_Color){0, 0, 0, 0});

        Tether_GUID txt = tether_widget_get_child_at(seg, 0);
        if (txt != TETHER_INVALID_GUID) {
            tether_ui_set_text_color(txt, is_selected ? (Tether_Color){255, 255, 255, 255} : (Tether_Color){175, 175, 175, 255});
        }
        segment_handle_feedback(seg, &(Tether_Event){ .type = TETHER_EVENT_CHANGE }, NULL);
    }
}

Tether_GUID tether_ui_segmented_bar(Tether_GUID parent) {
    Tether_GUID bar = tether_list_create(parent);

    Tether_Shape *s = (Tether_Shape*)tether_ecs_add_component(bar, TETHER_COMPONENT_SHAPE);
    if (s) {
        s->handle = TETHER_INVALID_HANDLE;
    }

    /* Shape & Panel layout styling */
    tether_ui_set_size(bar, 0.0f, 36.0f);
    tether_ui_set_container(bar, TETHER_FLOW_ROW);
    tether_ui_set_gap(bar, 2.0f, 0.0f);
    tether_ui_set_padding(bar, 3.0f, 3.0f, 3.0f, 3.0f);
    tether_ui_set_color(bar, (Tether_Color){32, 32, 32, 255});
    tether_ui_set_border(bar, 1.0f, (Tether_Color){75, 75, 75, 255}, 7.0f);

    return bar;
}

void tether_ui_set_segmented_bar_flow(Tether_GUID bar, Tether_Flow flow) {
    if (flow != TETHER_FLOW_ROW && flow != TETHER_FLOW_COLUMN) return;
    if (!tether_ecs_get_component(bar, TETHER_COMPONENT_SELECTION_STATE)) return;
    tether_ui_set_container(bar, flow);
    tether_ui_set_gap(bar, flow == TETHER_FLOW_ROW ? 2.0f : 0.0f,
                      flow == TETHER_FLOW_COLUMN ? 2.0f : 0.0f);
    tether_ui_set_size(bar, 0.0f, flow == TETHER_FLOW_ROW ? 36.0f : 0.0f);

    int count = tether_list_get_count(bar);
    for (int i = 0; i < count; ++i) {
        Tether_GUID segment = tether_list_get_item(bar, i);
        if (segment == TETHER_INVALID_GUID) break;
        tether_ui_set_size(segment, 0.0f, flow == TETHER_FLOW_COLUMN ? 30.0f : 0.0f);
        tether_ui_set_flex(segment, flow == TETHER_FLOW_ROW ? 1.0f : 0.0f);
    }
}

/* ========================================================================== */
/* 2. Internal Event Handling                                                 */
/* ========================================================================== */

static void segment_handle_feedback(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    int offset = tether_ui_interaction_offset(entity);
    if (offset == 0) {
        tether_ui_reset_active_color(entity);
        return;
    }

    Tether_Color base = tether_ui_get_color(entity);
    if (base.a == 0) {
        /* Unselected segments are transparent; use the bar color for visible feedback. */
        Tether_GUID bar = tether_widget_get_parent(entity);
        base = tether_ui_get_color(bar);
    }
    tether_ui_set_active_color(entity, tether_ui_adjust_color(base, offset));
}

static void segment_handle_click(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    Tether_GUID bar = tether_widget_get_parent(entity);
    if (bar == TETHER_INVALID_GUID) return;

    int count = tether_list_get_count(bar);
    for (int i = 0; i < count; i++) {
        if (tether_list_get_item(bar, i) == entity) {
            tether_ui_set_segment_selected(bar, i);
            break;
        }
    }
}

/* ========================================================================== */
/* 3. Operations (Item Management & Selection)                                */
/* ========================================================================== */

Tether_GUID tether_ui_add_segment(Tether_GUID bar, const char *label) {
    if (bar == TETHER_INVALID_GUID) return TETHER_INVALID_GUID;

    Tether_GUID seg = tether_list_add_item(bar);
    tether_ui_set_container(seg, TETHER_FLOW_ROW);
    tether_ui_set_content_align(seg, TETHER_ALIGN_CENTER, TETHER_ALIGN_CENTER);
    tether_ui_set_padding(seg, 6.0f, 14.0f, 6.0f, 14.0f);
    Tether_Layout *bar_layout = tether_ecs_get_component(bar, TETHER_COMPONENT_LAYOUT);
    bool vertical = bar_layout && bar_layout->flow == TETHER_FLOW_COLUMN;
    tether_ui_set_size(seg, 0.0f, vertical ? 30.0f : 0.0f);
    tether_ui_set_flex(seg, vertical ? 0.0f : 1.0f);

    tether_ecs_add_component(seg, TETHER_COMPONENT_SHAPE);
    tether_ui_set_color(seg, (Tether_Color){0, 0, 0, 0});
    tether_ui_set_border(seg, 0.0f, (Tether_Color){0, 0, 0, 0}, 5.0f);

    tether_widget_add_interaction(seg, TETHER_HIT_BLOCK);

    /* Child 0 of segment: Label */
    Tether_GUID txt = tether_ui_text(seg);
    tether_ui_set_text(txt, label ? label : "");
    tether_ui_set_text_size(txt, 14.0f);
    tether_ui_set_text_color(txt, (Tether_Color){175, 175, 175, 255});
    tether_ui_set_text_align(txt, TETHER_ALIGN_CENTER, TETHER_ALIGN_CENTER);

    tether_bind_event(seg, TETHER_EVENT_CLICK, segment_handle_click, NULL);
    tether_bind_event(seg, TETHER_EVENT_HOVER_ENTER, segment_handle_feedback, NULL);
    tether_bind_event(seg, TETHER_EVENT_HOVER_EXIT, segment_handle_feedback, NULL);
    tether_bind_event(seg, TETHER_EVENT_PRESS, segment_handle_feedback, NULL);
    tether_bind_event(seg, TETHER_EVENT_RELEASE, segment_handle_feedback, NULL);

    segmented_bar_update(bar);
    return seg;
}

void tether_ui_set_segment(Tether_GUID bar, int index, const char *label) {
    Tether_GUID seg = tether_list_get_item(bar, index);
    if (seg == TETHER_INVALID_GUID) return;
    Tether_GUID txt = tether_widget_get_child_at(seg, 0);
    if (txt != TETHER_INVALID_GUID && label) {
        tether_ui_set_text(txt, label);
    }
}

const char* tether_ui_get_segment(Tether_GUID bar, int index) {
    Tether_GUID seg = tether_list_get_item(bar, index);
    if (seg == TETHER_INVALID_GUID) return NULL;
    Tether_GUID txt = tether_widget_get_child_at(seg, 0);
    return txt != TETHER_INVALID_GUID ? tether_ui_get_text(txt) : NULL;
}

void tether_ui_remove_segment(Tether_GUID bar, int index) {
    tether_list_remove_item(bar, index);
    segmented_bar_update(bar);
}

void tether_ui_clear_segments(Tether_GUID bar) {
    tether_list_clear(bar);
    segmented_bar_update(bar);
}

void tether_ui_set_segment_selected(Tether_GUID bar, int selected_index) {
    tether_list_set_selected(bar, selected_index);
    segmented_bar_update(bar);
    tether_dispatch_event(bar, TETHER_EVENT_CHANGE);
}

int tether_ui_get_segment_selected(Tether_GUID bar) {
    return tether_list_get_selected(bar);
}

void tether_ui_unselect_segment(Tether_GUID bar) {
    tether_list_unselect(bar);
    segmented_bar_update(bar);
}
