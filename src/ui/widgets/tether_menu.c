#include "tether/ui/widgets/tether_menu.h"
#include "tether/ui/widgets/tether_button.h"
#include "tether/ui/widgets/tether_text.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_list.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_input.h"
#include "tether/engine/tether_engine.h"
#include <string.h>

static Tether_GUID s_active_menu = TETHER_INVALID_GUID;

/* ========================================================================== */
/* Helpers                                                                    */
/* ========================================================================== */

static int menu_find_item_index(Tether_GUID menu, Tether_GUID target) {
    Tether_Hierarchy *mh = (Tether_Hierarchy*)tether_ecs_get_component(menu, TETHER_COMPONENT_HIERARCHY);
    if (!mh) return -1;
    int idx = 0;
    Tether_GUID child = mh->first_child;
    while (child != TETHER_INVALID_GUID) {
        if (child == target) return idx;
        Tether_Hierarchy *ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
        if (ch && ch->first_child == target) return idx;
        child = ch ? ch->next_sibling : TETHER_INVALID_GUID;
        idx++;
    }
    return -1;
}

static bool menu_is_entity_in_menu_or_owner(Tether_GUID menu, Tether_GUID entity) {
    if (entity == TETHER_INVALID_GUID || menu == TETHER_INVALID_GUID) return false;
    Tether_Hierarchy *mh = (Tether_Hierarchy*)tether_ecs_get_component(menu, TETHER_COMPONENT_HIERARCHY);
    Tether_GUID owner = (mh && mh->parent != TETHER_INVALID_GUID) ? mh->parent : menu;
    Tether_Hierarchy *owner_h = tether_ecs_get_component(owner, TETHER_COMPONENT_HIERARCHY);
    if (owner_h && owner_h->parent == tether_ecs_get_overlay_root()) {
        owner = menu;
    }

    Tether_GUID curr = entity;
    while (curr != TETHER_INVALID_GUID) {
        if (curr == menu || curr == owner) return true;
        Tether_Hierarchy *h = (Tether_Hierarchy*)tether_ecs_get_component(curr, TETHER_COMPONENT_HIERARCHY);
        curr = h ? h->parent : TETHER_INVALID_GUID;
    }
    return false;
}

static void menu_handle_item_click(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;

    Tether_GUID menu = (Tether_GUID)(uintptr_t)user_data;
    int idx = menu_find_item_index(menu, entity);
    if (idx >= 0) {
        tether_ui_menu_set_selected(menu, idx);
        tether_dispatch_event(menu, TETHER_EVENT_CHANGE);
        if (tether_ecs_is_valid(menu)) tether_ui_menu_close(menu);
    }
}

static void menu_input_route(Tether_GUID menu, const Tether_Pointer_Event* event, Tether_GUID hit, Tether_GUID pressed) {
    Tether_GUID owner = tether_widget_get_parent(menu);
    if (!tether_widget_is_enabled(menu) ||
        (owner != TETHER_INVALID_GUID && !tether_widget_is_enabled(owner))) return;

    if (event->type == TETHER_POINTER_DOWN && event->button == TETHER_MOUSE_BUTTON_LEFT) {
        /* Outside click: pointer pressed outside the active menu and its owner collapses it immediately */
        if (!menu_is_entity_in_menu_or_owner(menu, hit)) {
            tether_ui_menu_close(menu);
        }
    }
    else if (event->type == TETHER_POINTER_UP && event->button == TETHER_MOUSE_BUTTON_LEFT) {
        int index = menu_find_item_index(menu, hit);
        if (pressed != hit && pressed != TETHER_INVALID_GUID && index >= 0 &&
            menu_is_entity_in_menu_or_owner(menu, pressed)) {
            Tether_GUID item = tether_widget_get_child_at(menu, index);
            Tether_Event click = { .type = TETHER_EVENT_CLICK, .button = event->button,
                                   .x = event->x, .y = event->y };
            tether_dispatch_event_data(item, &click);
        } else if (!menu_is_entity_in_menu_or_owner(menu, hit)) {
            tether_ui_menu_close(menu);
        }
    }
}

/* ========================================================================== */
/* Widget Creation                                                            */
/* ========================================================================== */

Tether_GUID tether_ui_menu(Tether_GUID parent) {
    Tether_GUID menu = tether_list_create(parent);
    tether_ecs_add_component(menu, TETHER_COMPONENT_SHAPE);
    tether_ui_set_position(menu, TETHER_POSITION_ABSOLUTE);
    tether_ui_set_container(menu, TETHER_FLOW_COLUMN);
    tether_ui_set_gap(menu, 0.0f, 2.0f);
    tether_ui_set_padding(menu, 4.0f, 4.0f, 4.0f, 4.0f);
    tether_ui_set_color(menu, (Tether_Color){29, 29, 29, 255});
    tether_ui_set_border(menu, 1.0f, (Tether_Color){79, 79, 79, 255}, 6.0f);
    tether_widget_add_interaction(menu, TETHER_HIT_BLOCK);
    tether_widget_set_visibility(menu, TETHER_COLLAPSED);

    return menu;
}

/* ========================================================================== */
/* Item Management                                                            */
/* ========================================================================== */

void tether_ui_menu_set_items(Tether_GUID menu, const char **items, int count) {
    if (menu == TETHER_INVALID_GUID || !items || count <= 0) return;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(menu, TETHER_COMPONENT_SELECTION_STATE);
    if (!sel) return;

    /* Destroy existing items */
    tether_list_clear(menu);

    float item_h = 30.0f;
    float gap_y = 2.0f;
    float pad_y = 8.0f;
    float total_h = count * item_h + (count > 1 ? (count - 1) * gap_y : 0.0f) + pad_y;

    tether_ui_set_size(menu, 0.0f, total_h);

    for (int i = 0; i < count; i++) {
        Tether_GUID item = tether_ui_button(menu);
        tether_ui_set_container(item, TETHER_FLOW_ROW);
        tether_ui_set_size(item, 0.0f, item_h);
        tether_ui_set_padding(item, 4.0f, 10.0f, 4.0f, 10.0f);
        tether_ui_set_content_align(item, TETHER_ALIGN_START, TETHER_ALIGN_CENTER);
        tether_ui_set_color(item, (Tether_Color){42, 42, 42, 255});
        tether_ui_set_border(item, 0.0f, (Tether_Color){0, 0, 0, 0}, 4.0f);
        tether_ui_set_flex(item, 0.0f);

        Tether_GUID label = tether_widget_get_child_at(item, 0);
        if (label != TETHER_INVALID_GUID) {
            tether_ui_set_text(label, items[i]);
            tether_ui_set_text_size(label, 13.0f);
            tether_ui_set_text_color(label, (Tether_Color){210, 210, 210, 255});
            tether_ui_set_text_align(label, TETHER_ALIGN_START, TETHER_ALIGN_CENTER);
            tether_ui_set_flex(label, 1.0f);
        }

        tether_bind_event(item, TETHER_EVENT_CLICK, menu_handle_item_click, (void*)(uintptr_t)menu);
    }

    /* Realize menu buttons once during setup */
    tether_engine_realize(menu);

    sel = tether_ecs_get_component(menu, TETHER_COMPONENT_SELECTION_STATE);
    if (!sel) return;
    sel->count = count;
    if (sel->selected_index < 0 || sel->selected_index >= count) {
        sel->selected_index = 0;
    }
}

void tether_ui_menu_set_selected(Tether_GUID menu, int selected_index) {
    if (menu == TETHER_INVALID_GUID) return;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(menu, TETHER_COMPONENT_SELECTION_STATE);
    if (sel && selected_index >= 0 && selected_index < sel->count) {
        sel->selected_index = selected_index;
    }
}

int tether_ui_menu_get_selected(Tether_GUID menu) {
    if (menu == TETHER_INVALID_GUID) return -1;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(menu, TETHER_COMPONENT_SELECTION_STATE);
    return sel ? sel->selected_index : -1;
}

int tether_ui_menu_get_item_count(Tether_GUID menu) {
    if (menu == TETHER_INVALID_GUID) return 0;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(menu, TETHER_COMPONENT_SELECTION_STATE);
    return sel ? sel->count : 0;
}

const char* tether_ui_menu_get_item_text(Tether_GUID menu, int index) {
    if (menu == TETHER_INVALID_GUID || index < 0) return NULL;
    Tether_GUID item = tether_widget_get_child_at(menu, index);
    if (item == TETHER_INVALID_GUID) return NULL;
    Tether_GUID label = tether_widget_get_child_at(item, 0);
    return label != TETHER_INVALID_GUID ? tether_ui_get_text(label) : NULL;
}

/* ========================================================================== */
/* Positioning & Lifecycle                                                    */
/* ========================================================================== */

void tether_ui_menu_position_relative(Tether_GUID menu, Tether_GUID anchor_header, float offset_y) {
    (void)anchor_header;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(menu, TETHER_COMPONENT_SELECTION_STATE);
    int count = sel ? sel->count : 0;
    float item_h = 30.0f;
    float gap_y = 2.0f;
    float pad_y = 8.0f;
    float total_h = count * item_h + (count > 1 ? (count - 1) * gap_y : 0.0f) + pad_y;

    tether_ui_set_position(menu, TETHER_POSITION_ABSOLUTE);
    tether_ui_set_anchor(menu, 0.0f, 1.0f, 1.0f, 1.0f);
    tether_ui_set_offsets(menu, offset_y, 0.0f, -(offset_y + total_h), 0.0f);
    tether_ui_set_size(menu, 0.0f, total_h);
}

void tether_ui_menu_position_at(Tether_GUID menu, float x, float y, float width) {
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(menu, TETHER_COMPONENT_SELECTION_STATE);
    int count = sel ? sel->count : 0;
    float item_h = 30.0f;
    float gap_y = 2.0f;
    float pad_y = 8.0f;
    float total_h = count * item_h + (count > 1 ? (count - 1) * gap_y : 0.0f) + pad_y;

    tether_ui_set_position(menu, TETHER_POSITION_ABSOLUTE);
    tether_ui_set_anchor(menu, 0.0f, 0.0f, 0.0f, 0.0f);
    tether_ui_set_offsets(menu, y, -(x + width), -(y + total_h), x);
    tether_ui_set_size(menu, width, total_h);
}

void tether_ui_menu_open(Tether_GUID menu) {
    if (menu == TETHER_INVALID_GUID) return;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(menu, TETHER_COMPONENT_SELECTION_STATE);
    if (!sel) return;

    if (s_active_menu != TETHER_INVALID_GUID && s_active_menu != menu) {
        tether_ui_menu_close(s_active_menu);
        sel = tether_ecs_get_component(menu, TETHER_COMPONENT_SELECTION_STATE);
        if (!sel) return;
    }
    if (sel->is_open) {
        s_active_menu = menu;
        tether_input_set_route(menu, menu_input_route);
        return;
    }

    sel->is_open = true;
    s_active_menu = menu;
    tether_widget_set_visibility(menu, TETHER_VISIBLE);
    tether_input_set_route(menu, menu_input_route);
    tether_dispatch_event(menu, TETHER_EVENT_CHANGE);
}

void tether_ui_menu_close(Tether_GUID menu) {
    if (menu == TETHER_INVALID_GUID) return;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(menu, TETHER_COMPONENT_SELECTION_STATE);
    if (!sel) return;

    if (!sel->is_open) return;
    sel->is_open = false;
    if (s_active_menu == menu) {
        s_active_menu = TETHER_INVALID_GUID;
        tether_input_clear_route(menu);
    }
    tether_widget_set_visibility(menu, TETHER_COLLAPSED);
    tether_dispatch_event(menu, TETHER_EVENT_CHANGE);
}

bool tether_ui_menu_is_open(Tether_GUID menu) {
    if (menu == TETHER_INVALID_GUID) return false;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(menu, TETHER_COMPONENT_SELECTION_STATE);
    return sel ? sel->is_open : false;
}
