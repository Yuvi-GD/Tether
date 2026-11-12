#ifndef TETHER_TEXT_H
#define TETHER_TEXT_H

#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Text Entity Creation & Lifecycle */
Tether_GUID tether_ui_text(Tether_GUID parent);

/* Text String Accessors */
bool tether_ui_set_text(Tether_GUID entity, const char *string);
const char* tether_ui_get_text(Tether_GUID entity);

/* Text Style Modifiers */
void tether_ui_set_text_color(Tether_GUID entity, Tether_Color color);
void tether_ui_set_text_size(Tether_GUID entity, float font_size);
void tether_ui_set_text_align(Tether_GUID entity, Tether_Align align_x, Tether_Align align_y);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_TEXT_H */
