#include "sandbox_widget.h"
#include "sandbox_events.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_registry.h"
#include <string.h>

Tether_GUID create_my_c_modal(Tether_GUID parent) {
    /* 1. Create Background Overlay Panel */
    Tether_GUID overlay = tether_create_widget("Panel", parent);
    Tether_LayoutNode* ol = (Tether_LayoutNode*)tether_ecs_get_component(overlay, TETHER_COMPONENT_LAYOUT_NODE);
    if (ol) {
        ol->flow = TETHER_FLOW_COLUMN;
        ol->content_align_x = TETHER_ALIGN_CENTER;
        ol->content_align_y = TETHER_ALIGN_CENTER;
        ol->hit_behavior = TETHER_HIT_BLOCK; /* Block clicks to background */
        strncpy(ol->id, "c_modal_overlay", sizeof(ol->id) - 1);
    }
    
    Tether_Style* os = (Tether_Style*)tether_ecs_get_component(overlay, TETHER_COMPONENT_STYLE);
    if (os) os->bg_color = (Tether_Color){0, 0, 0, 150}; /* Semi-transparent black */
    
    Tether_AnchorSlot* anchor = (Tether_AnchorSlot*)tether_ecs_get_component(overlay, TETHER_COMPONENT_ANCHOR_SLOT);
    if (anchor) {
        anchor->anchor_min = (Tether_Vec2){0.0f, 0.0f};
        anchor->anchor_max = (Tether_Vec2){1.0f, 1.0f};
    }
    
    /* 2. Create Inner Popup Panel */
    Tether_GUID popup = tether_create_widget("Panel", overlay);
    Tether_LayoutNode* pl = (Tether_LayoutNode*)tether_ecs_get_component(popup, TETHER_COMPONENT_LAYOUT_NODE);
    if (pl) {
        pl->flow = TETHER_FLOW_COLUMN;
        pl->padding = (Tether_Edges){40, 40, 40, 40};
        pl->gap = (Tether_Vec2){0, 30};
        pl->content_align_x = TETHER_ALIGN_CENTER;
        pl->content_align_y = TETHER_ALIGN_CENTER;
    }
    
    Tether_Style* ps = (Tether_Style*)tether_ecs_get_component(popup, TETHER_COMPONENT_STYLE);
    if (ps) {
        ps->bg_color = (Tether_Color){240, 245, 250, 255}; /* Light grayish blue */
    }
    
    Tether_FlexSlot* pf = (Tether_FlexSlot*)tether_ecs_get_component(popup, TETHER_COMPONENT_FLEX_SLOT);
    if (pf) {
        pf->fill_ratio = 0.0f; /* Don't stretch, size to content */
    }
    
    /* 3. Create Text Label */
    Tether_GUID text = tether_create_widget("Text", popup);
    Tether_TextStyle* ts = (Tether_TextStyle*)tether_ecs_get_component(text, TETHER_COMPONENT_TEXT_STYLE);
    if (ts) {
        ts->font_size = 28.0f;
    }
    tether_ecs_set_text_string(text, "Welcome to the C Modal!");
    
    /* 4. Create Close Button */
    Tether_GUID btn = tether_create_widget("Button", popup);
    Tether_Style* bs = (Tether_Style*)tether_ecs_get_component(btn, TETHER_COMPONENT_STYLE);
    if (bs) {
        bs->bg_color = (Tether_Color){220, 53, 69, 255}; /* Bootstrap Danger Red */
    }
    
    /* Modify the button's automatically generated text child */
    Tether_Hierarchy* bh = (Tether_Hierarchy*)tether_ecs_get_component(btn, TETHER_COMPONENT_HIERARCHY);
    if (bh && bh->first_child != TETHER_INVALID_GUID) {
        tether_ecs_set_text_string(bh->first_child, "Close Modal");
    }
    
    /* Bind close event on button click (passing the overlay root so it destroys the whole modal) */
    tether_bind_event(btn, TETHER_EVENT_CLICK, on_close_c_modal, (void*)(uintptr_t)overlay);
    
    return overlay;
}
