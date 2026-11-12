#include "tether/ui/widgets/tether_dropdown.h"
#include "tether/ui/widgets/tether_menu.h"
#include "tether/ui/widgets/tether_button.h"
#include "tether/ui/widgets/tether_text.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/core/tether_components.h"
#include <stdlib.h>
#include <string.h>

#define DROPDOWN_CHILD_HEADER 0
#define DROPDOWN_CHILD_MENU   1

/* Forward declarations */
static void dropdown_handle_header_click(Tether_GUID entity, const Tether_Event *event_info, void *user_data);
static void dropdown_handle_menu_change(Tether_GUID entity, const Tether_Event *event_info, void *user_data);

/* ========================================================================== */
/* 1. Hierarchy Helpers                                                       */
/* ========================================================================== */

static Tether_GUID dropdown_get_header(Tether_GUID outer) {
    return tether_widget_get_child_at(outer, DROPDOWN_CHILD_HEADER);
}

static Tether_GUID dropdown_get_menu(Tether_GUID outer) {
    return tether_widget_get_child_at(outer, DROPDOWN_CHILD_MENU);
}

static Tether_GUID dropdown_get_header_label(Tether_GUID header) {
    return tether_widget_get_child_at(header, 0);
}

static Tether_GUID dropdown_get_arrow_text(Tether_GUID header) {
    return tether_widget_get_child_at(header, 1);
}

/* ========================================================================== */
/* 2. Visual Sync                                                             */
/* ========================================================================== */

static void dropdown_sync_visuals(Tether_GUID outer) {
    Tether_GUID header = dropdown_get_header(outer);
    Tether_GUID label = dropdown_get_header_label(header);
    Tether_GUID arrow = dropdown_get_arrow_text(header);
    Tether_GUID menu = dropdown_get_menu(outer);

    /* Rotate the same bracket right when closed and down when open. */
    if (arrow != TETHER_INVALID_GUID) {
        Tether_RenderTransform *rt = (Tether_RenderTransform*)tether_ecs_get_component(arrow, TETHER_COMPONENT_RENDER_TRANSFORM);
        if (rt) {
            rt->rotation_deg = tether_ui_menu_is_open(menu) ? -90.0f : 180.0f;
            rt->render_pivot = (Tether_Vec2){0.5f, 0.5f};
            rt->scale_x = 1.0f;
            rt->scale_y = 1.0f;
            tether_ecs_add_component(arrow, TETHER_COMPONENT_DIRTY_TEXT_CONTENT);
            tether_ecs_add_component(arrow, TETHER_COMPONENT_DIRTY_VISUAL);
        }
    }

    /* Update header label from menu's selected text */
    if (label != TETHER_INVALID_GUID && menu != TETHER_INVALID_GUID) {
        const char *item_txt = tether_ui_menu_get_item_text(menu, tether_ui_menu_get_selected(menu));
        const char *current = tether_ui_get_text(label);
        if (item_txt && (!current || strcmp(current, item_txt) != 0)) {
            /* Updating the label can move the item's text component. */
            size_t length = strlen(item_txt) + 1;
            char local[512];
            char *copy = length <= sizeof(local) ? local : malloc(length);
            if (copy) {
                memcpy(copy, item_txt, length);
                tether_ui_set_text(label, copy);
                if (copy != local) free(copy);
            }
        }
    }
}

/* ========================================================================== */
/* 3. Widget Creation                                                         */
/* ========================================================================== */

Tether_GUID tether_ui_dropdown(Tether_GUID parent) {
    /* Outer container: FLOW_NONE anchors header inside 36px box */
    Tether_GUID outer = tether_widget_create_container(parent);
    tether_ui_set_container(outer, TETHER_FLOW_NONE);
    tether_ui_set_size(outer, 0.0f, 36.0f);

    /* Child 0: Header toggle button */
    Tether_GUID header = tether_ui_button(outer);
    tether_ui_set_container(header, TETHER_FLOW_ROW);
    tether_ui_set_size(header, 0.0f, 36.0f);
    tether_ui_set_padding(header, 6.0f, 12.0f, 6.0f, 12.0f);
    tether_ui_set_content_align(header, TETHER_ALIGN_START, TETHER_ALIGN_CENTER);
    tether_ui_set_color(header, (Tether_Color){42, 42, 42, 255});
    tether_ui_set_border(header, 1.0f, (Tether_Color){79, 79, 79, 255}, 6.0f);
    tether_ui_set_anchor(header, 0.0f, 0.0f, 1.0f, 1.0f);
    tether_ui_set_offsets(header, 0.0f, 0.0f, 0.0f, 0.0f);

    /* Header text label (Child 0 of button) */
    Tether_GUID label = dropdown_get_header_label(header);
    if (label != TETHER_INVALID_GUID) {
        tether_ui_set_text(label, "Select...");
        tether_ui_set_text_size(label, 14.0f);
        tether_ui_set_text_color(label, (Tether_Color){225, 225, 225, 255});
        tether_ui_set_text_align(label, TETHER_ALIGN_START, TETHER_ALIGN_CENTER);
        tether_ui_set_flex(label, 1.0f);
    }

    /* Chevron bracket text (Child 1 of header, rotated "<") */
    Tether_GUID arrow = tether_ui_text(header);
    tether_ui_set_text(arrow, "<");
    tether_ui_set_text_size(arrow, 24.0f);
    tether_ui_set_text_color(arrow, (Tether_Color){145, 145, 145, 255});
    tether_ui_set_text_align(arrow, TETHER_ALIGN_CENTER, TETHER_ALIGN_CENTER);
    Tether_RenderTransform *rt = (Tether_RenderTransform*)tether_ecs_get_component(arrow, TETHER_COMPONENT_RENDER_TRANSFORM);
    if (rt) {
        rt->render_pivot = (Tether_Vec2){0.5f, 0.5f};
        rt->scale_x = 1.0f;
        rt->scale_y = 1.0f;
        rt->rotation_deg = 0.0f;
    }

    /* Child 1: Menu floating panel */
    Tether_GUID menu = tether_ui_menu(outer);
    tether_ui_menu_position_relative(menu, header, 2.0f);

    tether_bind_event(header, TETHER_EVENT_PRESS, dropdown_handle_header_click, (void*)(uintptr_t)outer);
    tether_bind_event(menu, TETHER_EVENT_CHANGE, dropdown_handle_menu_change, (void*)(uintptr_t)outer);

    dropdown_sync_visuals(outer);
    return outer;
}

/* ========================================================================== */
/* 4. Event Handlers                                                          */
/* ========================================================================== */

static void dropdown_handle_header_click(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)entity;
    Tether_GUID outer = (Tether_GUID)(uintptr_t)user_data;
    tether_ui_set_dropdown_open(outer, !tether_ui_is_dropdown_open(outer));
}

static void dropdown_handle_menu_change(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)entity;
    Tether_GUID outer = (Tether_GUID)(uintptr_t)user_data;
    dropdown_sync_visuals(outer);
    tether_dispatch_event_data(outer, event_info);
}

/* ========================================================================== */
/* 5. Setters & Getters                                                       */
/* ========================================================================== */

void tether_ui_set_dropdown_selected(Tether_GUID dropdown, int selected_index) {
    if (dropdown == TETHER_INVALID_GUID) return;
    Tether_GUID menu = dropdown_get_menu(dropdown);
    if (selected_index >= 0 && selected_index < tether_ui_menu_get_item_count(menu)) {
        tether_ui_menu_set_selected(menu, selected_index);
        dropdown_sync_visuals(dropdown);
        tether_dispatch_event(dropdown, TETHER_EVENT_CHANGE);
    }
}

int tether_ui_get_dropdown_selected(Tether_GUID dropdown) {
    return tether_ui_menu_get_selected(dropdown_get_menu(dropdown));
}

void tether_ui_set_dropdown_open(Tether_GUID dropdown, bool open) {
    if (dropdown == TETHER_INVALID_GUID) return;
    Tether_GUID menu = dropdown_get_menu(dropdown);
    if (menu != TETHER_INVALID_GUID) {
        if (open) {
            tether_ui_menu_open(menu);
        } else {
            tether_ui_menu_close(menu);
        }
    }
}

bool tether_ui_is_dropdown_open(Tether_GUID dropdown) {
    if (dropdown == TETHER_INVALID_GUID) return false;
    Tether_GUID menu = dropdown_get_menu(dropdown);
    return tether_ui_menu_is_open(menu);
}

void tether_ui_set_dropdown_options(Tether_GUID dropdown, const char **options, int count) {
    if (dropdown == TETHER_INVALID_GUID || !options || count <= 0) return;
    Tether_GUID header = dropdown_get_header(dropdown);
    Tether_GUID menu = dropdown_get_menu(dropdown);
    if (menu != TETHER_INVALID_GUID) {
        tether_ui_menu_set_items(menu, options, count);
        tether_ui_menu_position_relative(menu, header, 2.0f);
    }

    dropdown_sync_visuals(dropdown);
}

void tether_ui_set_dropdown_type(Tether_GUID dropdown, Tether_DropdownType type) {
    (void)type;
    dropdown_sync_visuals(dropdown);
}
