#include "sandbox_cards.h"

Tether_GUID sandbox_metric_card(Tether_GUID parent, const char *title, const char *default_val,
                                Tether_Color bg_color, Tether_Color accent_color,
                                Tether_GUID *out_val_entity) {
    Tether_GUID card = tether_ui_column(parent, 7.0f);
    tether_ui_set_padding(card, 10.0f, 12.0f, 10.0f, 12.0f);
    tether_ui_set_color(card, bg_color);
    tether_ui_set_border(card, 0.0f, (Tether_Color){0, 0, 0, 0}, 8.0f);
    tether_ui_set_flex(card, 1.0f);

    /* Caption */
    tether_ui_text(card, title, 15.0f, (Tether_Color){181, 199, 220, 255});

    /* Value */
    Tether_GUID val_id = tether_ui_text(card, default_val ? default_val : "--", 24.0f, accent_color);
    if (out_val_entity) {
        *out_val_entity = val_id;
    }

    return card;
}

Tether_GUID sandbox_status_badge(Tether_GUID parent, const char *status_text,
                                 Tether_Color badge_color, Tether_GUID *out_badge_text) {
    Tether_GUID panel = tether_ui_column(parent, 4.0f);
    tether_ui_set_padding(panel, 10.0f, 14.0f, 10.0f, 14.0f);
    tether_ui_set_color(panel, (Tether_Color){28, 42, 56, 255});
    tether_ui_set_border(panel, 0.0f, (Tether_Color){0, 0, 0, 0}, 8.0f);

    Tether_GUID txt = tether_ui_text(panel, status_text ? status_text : "READY", 24.0f, badge_color);
    if (out_badge_text) {
        *out_badge_text = txt;
    }

    return panel;
}

void sandbox_set_status_badge(Tether_GUID badge_text_entity, const char *status_text, Tether_Color color) {
    tether_ui_set_text(badge_text_entity, status_text);
    tether_ui_set_text_color(badge_text_entity, color);
}
