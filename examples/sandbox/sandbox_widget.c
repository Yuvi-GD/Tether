#include "sandbox_widget.h"
#include "sandbox_events.h"
#include "tether/ui/tether_ui.h"
#include "tether/core/tether_registry.h"
#include "tether/core/tether_components.h"

Tether_GUID create_my_c_modal(Tether_GUID parent) {
    /* 1. Background Overlay Panel */
    Tether_GUID overlay = tether_ui_column(parent, 0.0f);
    tether_ui_set_color(overlay, (Tether_Color){0, 0, 0, 150});
    tether_ui_set_flex(overlay, 1.0f);
    tether_registry_alias_entity(overlay, "c_modal_overlay");

    Tether_Layout *ol = (Tether_Layout*)tether_ecs_get_component(overlay, TETHER_COMPONENT_LAYOUT);
    if (ol) {
        ol->content_align_x = TETHER_ALIGN_CENTER;
        ol->content_align_y = TETHER_ALIGN_CENTER;
    }
    Tether_Interactable *oi = (Tether_Interactable*)tether_ecs_get_component(overlay, TETHER_COMPONENT_INTERACTABLE);
    if (oi) oi->hit_behavior = TETHER_HIT_BLOCK;

    Tether_AnchorSlot *anchor = (Tether_AnchorSlot*)tether_ecs_get_component(overlay, TETHER_COMPONENT_ANCHOR_SLOT);
    if (anchor) {
        anchor->anchor_min = (Tether_Vec2){0.0f, 0.0f};
        anchor->anchor_max = (Tether_Vec2){1.0f, 1.0f};
    }

    /* 2. Inner Popup Panel */
    Tether_GUID popup = tether_ui_column(overlay, 30.0f);
    tether_ui_set_padding(popup, 40.0f, 40.0f, 40.0f, 40.0f);
    tether_ui_set_color(popup, (Tether_Color){240, 245, 250, 255});
    tether_ui_set_flex(popup, 0.0f);

    Tether_Layout *pl = (Tether_Layout*)tether_ecs_get_component(popup, TETHER_COMPONENT_LAYOUT);
    if (pl) {
        pl->content_align_x = TETHER_ALIGN_CENTER;
        pl->content_align_y = TETHER_ALIGN_CENTER;
    }

    /* 3. Text & Close Button */
    tether_ui_text(popup, "Welcome to the C Modal!", 32.0f, (Tether_Color){30, 30, 30, 255});
    tether_ui_button(popup, "Close Modal", 20.0f, (Tether_Color){220, 53, 69, 255},
                     on_close_c_modal, (void*)(uintptr_t)overlay);

    return overlay;
}
