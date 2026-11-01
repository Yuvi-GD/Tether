#ifndef SANDBOX_CARDS_H
#define SANDBOX_CARDS_H

#include "tether/ui/tether_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Create a 2-level metric card (caption on top, large value below) */
Tether_GUID sandbox_metric_card(Tether_GUID parent, const char *title, const char *default_val,
                                Tether_Color bg_color, Tether_Color accent_color,
                                Tether_GUID *out_val_entity);

/* Create a status badge / pill */
Tether_GUID sandbox_status_badge(Tether_GUID parent, const char *status_text,
                                 Tether_Color badge_color, Tether_GUID *out_badge_text);

/* Update status badge text and color */
void sandbox_set_status_badge(Tether_GUID badge_text_entity, const char *status_text, Tether_Color color);

#ifdef __cplusplus
}
#endif

#endif /* SANDBOX_CARDS_H */
