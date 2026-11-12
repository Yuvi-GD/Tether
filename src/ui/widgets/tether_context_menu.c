#include "tether/ui/widgets/tether_context_menu.h"
#include "tether/ui/widgets/tether_menu.h"
#include "tether/backends/tether_backend.h"
#include "tether/core/tether_components.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"

Tether_GUID tether_ui_context_menu(Tether_GUID parent) {
    return tether_ui_menu(parent);
}

void tether_ui_context_menu_open_at(Tether_GUID menu, float x, float y) {
    if (!tether_ecs_is_valid(menu)) return;

    int count = tether_ui_menu_get_item_count(menu);
    if (count <= 0) return;

    uint32_t viewport_width = 0, viewport_height = 0;
    if (g_backend.system && g_backend.system->get_resolution) {
        g_backend.system->get_resolution(&viewport_width, &viewport_height);
    }
    Tether_GUID host = tether_widget_get_parent(menu);
    if (tether_widget_get_parent(host) == tether_ecs_get_overlay_root()) {
        Tether_SlotTransform *slot = tether_ecs_get_component(host, TETHER_COMPONENT_SLOT_TRANSFORM);
        if (slot && viewport_width && viewport_height) {
            slot->x = 0.0f;
            slot->y = 0.0f;
            slot->width = (float)viewport_width;
            slot->height = (float)viewport_height;
        }
    }

    float width = 0.0f;
    for (int i = 0; i < count; ++i) {
        const char *text = tether_ui_menu_get_item_text(menu, i);
        float text_width = 0.0f, text_height = 0.0f;
        if (text && g_backend.text && g_backend.text->measure_text) {
            g_backend.text->measure_text(text, 0, 13.0f, 0.0f, &text_width, &text_height);
        }
        if (text_width > width) width = text_width;
    }
    width += 28.0f;
    float height = count * 30.0f + (count > 1 ? (count - 1) * 2.0f : 0.0f) + 8.0f;
    if (viewport_width && width > (float)viewport_width) width = (float)viewport_width;
    if (viewport_height && height > (float)viewport_height) height = (float)viewport_height;
    if (viewport_width && x + width > (float)viewport_width) x = (float)viewport_width - width;
    if (viewport_height && y + height > (float)viewport_height) y = (float)viewport_height - height;
    if (x < 0.0f) x = 0.0f;
    if (y < 0.0f) y = 0.0f;

    tether_ui_menu_position_at(menu, x, y, width);
    tether_ui_set_offsets(menu, y, -(x + width), -(y + height), x);
    tether_ui_set_size(menu, width, height);
    tether_ui_menu_open(menu);
}
