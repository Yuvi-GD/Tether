#include "tether/ui/tether_list.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"

TUID tether_list_create(TUID parent) {
    TUID list = tether_widget_create_container(parent);
    tether_ecs_add_component(list, TETHER_COMPONENT_SELECTION_STATE);
    return list;
}

TUID tether_list_add_item(TUID list) {
    if (list == TETHER_INVALID_TUID) return TETHER_INVALID_TUID;
    TUID item = tether_widget_create_container(list);
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(list, TETHER_COMPONENT_SELECTION_STATE);
    if (sel) {
        sel->count = tether_widget_get_child_count(list);
    }
    return item;
}

void tether_list_remove_item(TUID list, int index) {
    if (list == TETHER_INVALID_TUID || index < 0) return;
    TUID item = tether_widget_get_child_at(list, index);
    if (item == TETHER_INVALID_TUID) return;

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

void tether_list_clear(TUID list) {
    if (list == TETHER_INVALID_TUID) return;
    TUID item = tether_widget_get_child_at(list, -1);
    while (item != TETHER_INVALID_TUID) {
        tether_widget_destroy(item);
        item = tether_widget_get_child_at(list, -1);
    }
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(list, TETHER_COMPONENT_SELECTION_STATE);
    if (sel) {
        sel->selected_index = -1;
        sel->count = 0;
    }
}

TUID tether_list_get_item(TUID list, int index) {
    return tether_widget_get_child_at(list, index);
}

int tether_list_get_count(TUID list) {
    return tether_widget_get_child_count(list);
}

void tether_list_set_selected(TUID list, int index) {
    if (list == TETHER_INVALID_TUID) return;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(list, TETHER_COMPONENT_SELECTION_STATE);
    if (!sel) return;

    int count = tether_widget_get_child_count(list);
    sel->count = count;
    if (index >= -1 && (count == 0 || index < count)) {
        sel->selected_index = index;
    }
}

int tether_list_get_selected(TUID list) {
    if (list == TETHER_INVALID_TUID) return -1;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(list, TETHER_COMPONENT_SELECTION_STATE);
    return sel ? sel->selected_index : -1;
}

void tether_list_unselect(TUID list) {
    tether_list_set_selected(list, -1);
}
