#include "tether/ui/widgets/tether_switcher.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"
#include "tether/engine/tether_engine.h"

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

Tether_GUID tether_ui_switcher(Tether_GUID parent) {
    Tether_GUID container = tether_widget_create_container(parent);
    tether_ui_set_container(container, TETHER_FLOW_COLUMN);
    tether_ui_set_flex(container, 1.0f);

    /* Attach ECS SelectionState to switcher container */
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_add_component(container, TETHER_COMPONENT_SELECTION_STATE);
    if (sel) {
        sel->selected_index = 0;
    }

    return container;
}

/* ========================================================================== */
/* 2. Operations (View Management & Activation)                               */
/* ========================================================================== */

Tether_GUID tether_ui_switcher_add_view(Tether_GUID switcher) {
    if (switcher == TETHER_INVALID_GUID) return TETHER_INVALID_GUID;
    Tether_Hierarchy *h = (Tether_Hierarchy*)tether_ecs_get_component(switcher, TETHER_COMPONENT_HIERARCHY);
    bool is_first = (!h || h->first_child == TETHER_INVALID_GUID);
    Tether_GUID view = tether_widget_create_container(switcher);
    tether_ui_set_container(view, TETHER_FLOW_COLUMN);
    tether_ui_set_flex(view, 1.0f);
    if (!is_first) {
        tether_widget_hide(view);
    } else {
        tether_widget_set_visibility(view, TETHER_VISIBLE);
    }

    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(switcher, TETHER_COMPONENT_SELECTION_STATE);
    if (sel) {
        sel->count++;
    }

    return view;
}

void tether_ui_switcher_show_index(Tether_GUID switcher, int active_index) {
    if (switcher == TETHER_INVALID_GUID || active_index < 0) return;
    Tether_Hierarchy *h = (Tether_Hierarchy*)tether_ecs_get_component(switcher, TETHER_COMPONENT_HIERARCHY);
    if (!h || (uint32_t)active_index >= h->child_count) return;

    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(switcher, TETHER_COMPONENT_SELECTION_STATE);
    if (sel) {
        sel->selected_index = active_index;
    }

    int idx = 0;
    Tether_GUID child = h->first_child;
    while (child != TETHER_INVALID_GUID) {
        if (idx == active_index) {
            tether_widget_show(child);
        } else {
            tether_widget_hide(child);
        }
        Tether_Hierarchy *ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
        child = ch ? ch->next_sibling : TETHER_INVALID_GUID;
        idx++;
    }
    tether_dispatch_event(switcher, TETHER_EVENT_CHANGE);
}

int tether_ui_switcher_get_active(Tether_GUID switcher) {
    if (switcher == TETHER_INVALID_GUID) return 0;
    Tether_SelectionState *sel = (Tether_SelectionState*)tether_ecs_get_component(switcher, TETHER_COMPONENT_SELECTION_STATE);
    if (sel) return sel->selected_index;

    Tether_Hierarchy *h = (Tether_Hierarchy*)tether_ecs_get_component(switcher, TETHER_COMPONENT_HIERARCHY);
    if (!h) return 0;
    int idx = 0;
    Tether_GUID child = h->first_child;
    while (child != TETHER_INVALID_GUID) {
        Tether_Visibility *v = (Tether_Visibility*)tether_ecs_get_component(child, TETHER_COMPONENT_VISIBILITY);
        if (v && v->computed_state == TETHER_VISIBLE) {
            return idx;
        }
        Tether_Hierarchy *ch = (Tether_Hierarchy*)tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
        child = ch ? ch->next_sibling : TETHER_INVALID_GUID;
        idx++;
    }
    return 0;
}
