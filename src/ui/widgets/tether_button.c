#include "tether/ui/widgets/tether_button.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"

/* Forward declaration of internal event handlers */
static void button_handle_feedback(Tether_GUID entity, const Tether_Event *event_info, void *user_data);

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */


Tether_GUID tether_ui_button(Tether_GUID parent) {
    Tether_GUID btn = tether_ui_panel(parent);
    tether_ui_set_size(btn, 0.0f, 44.0f);
    tether_ui_set_padding(btn, 8.0f, 16.0f, 8.0f, 16.0f);
    tether_ui_set_content_align(btn, TETHER_ALIGN_CENTER, TETHER_ALIGN_CENTER);
    tether_ui_set_flex(btn, 1.0f);
    tether_ui_set_color(btn, (Tether_Color){45, 109, 190, 255});
    tether_ui_set_border(btn, 1.0f, (Tether_Color){55, 118, 193, 255}, 7.0f);

    tether_widget_add_interaction(btn, TETHER_HIT_BLOCK);

    Tether_GUID label = tether_ui_text(btn);
    tether_ui_set_text(label, "Button");
    tether_ui_set_text_size(label, 16.0f);
    tether_ui_set_text_color(label, (Tether_Color){235, 235, 235, 255});
    tether_ui_set_text_align(label, TETHER_ALIGN_CENTER, TETHER_ALIGN_CENTER);
    tether_ui_set_flex(label, 1.0f);

    /* Bind internal visual feedback events */
    tether_bind_event(btn, TETHER_EVENT_HOVER_ENTER, button_handle_feedback, NULL);
    tether_bind_event(btn, TETHER_EVENT_HOVER_EXIT, button_handle_feedback, NULL);
    tether_bind_event(btn, TETHER_EVENT_PRESS, button_handle_feedback, NULL);
    tether_bind_event(btn, TETHER_EVENT_RELEASE, button_handle_feedback, NULL);

    return btn;
}

/* ========================================================================== */
/* 2. Internal Event Handling                                                 */
/* ========================================================================== */

static void button_handle_feedback(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;

    int adjustment = tether_ui_interaction_offset(entity);
    if (adjustment == 0) {
        tether_ui_reset_active_color(entity);
        return;
    }

    Tether_Color base = tether_ui_get_color(entity);
    tether_ui_set_active_color(entity, tether_ui_adjust_color(base, adjustment));
}

/* ========================================================================== */
/* 3. Operations (Setters & Getters)                                          */
/* ========================================================================== */

void tether_ui_set_button_label(Tether_GUID btn, const char* label) {
    Tether_GUID lbl = tether_widget_get_child_at(btn, 0);
    if (lbl != TETHER_INVALID_GUID && label) {
        tether_ui_set_text(lbl, label);
    }
}

const char* tether_ui_get_button_label(Tether_GUID btn) {
    Tether_GUID lbl = tether_widget_get_child_at(btn, 0);
    return (lbl != TETHER_INVALID_GUID) ? tether_ui_get_text(lbl) : NULL;
}

void tether_ui_set_button_color(Tether_GUID btn, Tether_Color color) {
    tether_ui_set_color(btn, color);
}
