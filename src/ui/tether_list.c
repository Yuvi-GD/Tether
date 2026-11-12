#include "tether/ui/tether_list.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"

Tether_GUID tether_list_create(Tether_GUID parent) {
    Tether_GUID list = tether_widget_create_container(parent);
    tether_ecs_add_component(list, TETHER_COMPONENT_SELECTION_STATE);
    return list;
}

Tether_GUID tether_list_add_item(Tether_GUID list) {
    if (list == TETHER_INVALID_GUID) return TETHER_INVALID_GUID;
    Tether_GUID item = tether_widget_create_container(list);
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(list, TETHER_COMPONENT_SELECTION_STATE);
    if (sel) {
        sel->count = tether_widget_get_child_count(list);
    }
    return item;
}

void tether_list_remove_item(Tether_GUID list, int index) {
    if (list == TETHER_INVALID_GUID || index < 0) return;
    Tether_GUID item = tether_widget_get_child_at(list, index);
    if (item == TETHER_INVALID_GUID) return;

    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(list, TETHER_COMPONENT_SELECTION_STATE);
    if (sel) {
        if (sel->selected_index == index) {
            sel->selected_index = -1;
        } else if (sel->selected_index > index) {
            sel->selected_index--;
        }
    }

    tether_widget_destroy(item);

    sel = tether_ecs_get_component(list, TETHER_COMPONENT_SELECTION_STATE);
    if (sel) {
        sel->count = tether_widget_get_child_count(list);
    }
}

void tether_list_clear(Tether_GUID list) {
    if (list == TETHER_INVALID_GUID) return;
    Tether_GUID item = tether_widget_get_child_at(list, -1);
    while (item != TETHER_INVALID_GUID) {
        tether_widget_destroy(item);
        item = tether_widget_get_child_at(list, -1);
    }
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(list, TETHER_COMPONENT_SELECTION_STATE);
    if (sel) {
        sel->selected_index = -1;
        sel->count = 0;
    }
}

Tether_GUID tether_list_get_item(Tether_GUID list, int index) {
    return tether_widget_get_child_at(list, index);
}

int tether_list_get_count(Tether_GUID list) {
    return tether_widget_get_child_count(list);
}

void tether_list_set_selected(Tether_GUID list, int index) {
    if (list == TETHER_INVALID_GUID) return;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(list, TETHER_COMPONENT_SELECTION_STATE);
    if (!sel) return;

    int count = tether_widget_get_child_count(list);
    sel->count = count;
    if (index >= -1 && (count == 0 || index < count)) {
        sel->selected_index = index;
    }
}

int tether_list_get_selected(Tether_GUID list) {
    if (list == TETHER_INVALID_GUID) return -1;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(list, TETHER_COMPONENT_SELECTION_STATE);
    return sel ? sel->selected_index : -1;
}

void tether_list_unselect(Tether_GUID list) {
    tether_list_set_selected(list, -1);
}
