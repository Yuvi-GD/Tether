#include "sandbox_widgets_zoo.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/ui/widgets/tether_context_menu.h"
#include "tether/ui/widgets/tether_menu.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_input.h"
#include "tether/core/tether_registry.h"
#include "tether/engine/parser/tether_yaml.h"
#include <stdio.h>
#include <string.h>

extern Tether_GUID tether_ecs_get_main_root(void);

static Tether_GUID s_prog_bar = TETHER_INVALID_GUID;
static Tether_GUID s_slider_label = TETHER_INVALID_GUID;
static Tether_GUID s_toggle_label = TETHER_INVALID_GUID;
static Tether_GUID s_cb_label = TETHER_INVALID_GUID;
static Tether_GUID s_radio_label = TETHER_INVALID_GUID;
static Tether_GUID s_dd_label = TETHER_INVALID_GUID;
static Tether_GUID s_switcher = TETHER_INVALID_GUID;

static void on_slider_change(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    float val = tether_ui_get_slider_value(entity);
    if (s_prog_bar != TETHER_INVALID_GUID) {
        tether_ui_set_progress(s_prog_bar, val / 100.0f);
    }
    if (s_slider_label != TETHER_INVALID_GUID) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Value: %.0f%%", val);
        tether_ui_set_text(s_slider_label, buf);
    }
}

static void on_toggle_change(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    bool is_on = tether_ui_get_toggle(entity);
    if (s_toggle_label != TETHER_INVALID_GUID) {
        if (is_on) {
            tether_ui_set_text(s_toggle_label, "Example toggle: ON");
            tether_ui_set_text_color(s_toggle_label, (Tether_Color){145, 190, 245, 255});
        } else {
            tether_ui_set_text(s_toggle_label, "Example toggle: OFF");
            tether_ui_set_text_color(s_toggle_label, (Tether_Color){170, 180, 200, 255});
        }
    }
}

static void on_checkbox_change(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    bool checked = tether_ui_get_checked(entity);
    if (s_cb_label != TETHER_INVALID_GUID) {
        if (checked) {
            tether_ui_set_text(s_cb_label, "Example checkbox: checked");
            tether_ui_set_text_color(s_cb_label, (Tether_Color){90, 180, 250, 255});
        } else {
            tether_ui_set_text(s_cb_label, "Example checkbox: unchecked");
            tether_ui_set_text_color(s_cb_label, (Tether_Color){170, 180, 200, 255});
        }
    }
}

static void on_radio_change(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    int sel = tether_ui_get_radio_selected(entity);
    if (s_radio_label != TETHER_INVALID_GUID) {
        const char *desc = "Selected: High Precision";
        if (sel == 1) {
            desc = "Selected: Low Power";
        } else if (sel == 2) {
            desc = "Selected: Balanced";
        }
        tether_ui_set_text(s_radio_label, desc);
    }
}

static void on_dropdown_change(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    int sel = tether_ui_get_dropdown_selected(entity);
    if (s_dd_label != TETHER_INVALID_GUID) {
        const char *theme = "Selected example palette: Dark Slate";
        if (sel == 1) theme = "Selected example palette: Cyber Blue";
        else if (sel == 2) theme = "Selected example palette: Emerald Terminal";
        tether_ui_set_text(s_dd_label, theme);
    }
}

static const char *s_ctx_items[] = {
    "Example action A",
    "Example action B",
    "Example action C",
    "Example action D"
};

static void on_ctx_menu_select(Tether_GUID item, const Tether_Event *event_info, void *user_data) {
    (void)user_data;
    (void)event_info;
    const char *item_text = tether_ui_get_button_label(item);
    if (!item_text) return;
    if (s_dd_label != TETHER_INVALID_GUID) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Selected menu item: %s", item_text);
        tether_ui_set_text(s_dd_label, buf);
        tether_ui_set_text_color(s_dd_label, (Tether_Color){130, 220, 250, 255});
    }
}

static Tether_GUID s_context_view = TETHER_INVALID_GUID;
static Tether_GUID s_context_menu = TETHER_INVALID_GUID;
static Tether_GUID s_context_host = TETHER_INVALID_GUID;

static void on_root_context_menu(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)entity;
    (void)user_data;
    Tether_Visibility *visibility = tether_ecs_get_component(s_context_view, TETHER_COMPONENT_VISIBILITY);
    if (!visibility || visibility->local_state != TETHER_VISIBLE) return;
    tether_ui_context_menu_open_at(s_context_menu, event_info->x, event_info->y);
}

static void on_switcher_tab(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)event_info;
    (void)user_data;
    int sel = tether_ui_get_segment_selected(entity);
    if (s_switcher != TETHER_INVALID_GUID) {
        tether_ui_switcher_show_index(s_switcher, sel);
    }
}

static void on_demo_button(Tether_GUID entity, const Tether_Event *event_info, void *user_data) {
    (void)entity;
    (void)event_info;
    (void)user_data;
    tether_ui_set_text(tether_registry_find_entity("zoo_button_status"), "Enabled button clicked.");
}

void sandbox_widgets_zoo_cleanup(void) {
    tether_unbind_callback(tether_ecs_get_main_root(), TETHER_EVENT_CLICK, on_root_context_menu);
    if (tether_ecs_is_valid(s_context_host)) tether_widget_destroy(s_context_host);
    s_context_host = TETHER_INVALID_GUID;
    s_context_menu = TETHER_INVALID_GUID;
    s_context_view = TETHER_INVALID_GUID;
}

Tether_GUID sandbox_widgets_zoo_create(Tether_GUID parent) {
    /* Declarative YAML scene loading */
    Tether_GUID root = tether_yaml_load("UI/widgets.yaml", parent);
    if (root == TETHER_INVALID_GUID) {
        /* Fallback if YAML is not present */
        return TETHER_INVALID_GUID;
    }

    /* Keep the examples independent: each disabled widget stays visible. */
    const char *disabled_ids[] = {
        "zoo_button_disabled", "zoo_checkbox_disabled", "zoo_toggle_disabled", "zoo_slider_disabled",
        "zoo_dropdown_disabled", "zoo_radio_disabled",
        "zoo_progress_disabled", "zoo_collapsible_disabled", "zoo_switcher_tabs_disabled",
        "zoo_switcher_disabled"
    };
    for (unsigned i = 0; i < sizeof disabled_ids / sizeof *disabled_ids; ++i) {
        tether_widget_set_enabled(tether_registry_find_entity(disabled_ids[i]), false);
    }
    tether_bind_event(tether_registry_find_entity("zoo_button_enabled"), TETHER_EVENT_CLICK,
                      on_demo_button, NULL);
    tether_bind_event(tether_registry_find_entity("zoo_button_disabled"), TETHER_EVENT_CLICK,
                      on_demo_button, NULL);

    /* Receive right clicks on empty space as well as interactive widgets. */
    s_context_view = parent;
    s_context_host = tether_widget_create_container(tether_ecs_get_overlay_root());
    tether_ui_set_container(s_context_host, TETHER_FLOW_NONE);
    s_context_menu = tether_ui_context_menu(s_context_host);
    tether_ui_menu_set_items(s_context_menu, s_ctx_items, 4);
    for (int i = 0; i < 4; ++i) {
        tether_bind_event(tether_widget_get_child_at(s_context_menu, i),
                          TETHER_EVENT_CLICK, on_ctx_menu_select, NULL);
    }
    tether_unbind_callback(tether_ecs_get_main_root(), TETHER_EVENT_CLICK, on_root_context_menu);
    tether_bind_pointer_event(tether_ecs_get_main_root(), TETHER_EVENT_CLICK,
                              TETHER_MOUSE_BUTTON_RIGHT, on_root_context_menu, NULL);

    /* Bind dynamic control state and event callbacks via entity aliases */
    s_slider_label = tether_registry_find_entity("zoo_slider_label");
    s_prog_bar = tether_registry_find_entity("zoo_prog_bar");
    Tether_GUID slider = tether_registry_find_entity("zoo_slider");
    if (slider != TETHER_INVALID_GUID) {
        tether_bind_event(slider, TETHER_EVENT_CHANGE, on_slider_change, NULL);
    }

    s_toggle_label = tether_registry_find_entity("zoo_toggle_label");
    Tether_GUID toggle = tether_registry_find_entity("zoo_toggle");
    if (toggle != TETHER_INVALID_GUID) {
        tether_bind_event(toggle, TETHER_EVENT_CHANGE, on_toggle_change, NULL);
    }

    s_cb_label = tether_registry_find_entity("zoo_cb_label");
    Tether_GUID cb = tether_registry_find_entity("zoo_checkbox");
    if (cb != TETHER_INVALID_GUID) {
        tether_bind_event(cb, TETHER_EVENT_CHANGE, on_checkbox_change, NULL);
    }

    s_radio_label = tether_registry_find_entity("zoo_radio_label");
    Tether_GUID rad = tether_registry_find_entity("zoo_radio");
    if (rad != TETHER_INVALID_GUID) {
        tether_bind_event(rad, TETHER_EVENT_CHANGE, on_radio_change, NULL);
    }

    s_dd_label = tether_registry_find_entity("zoo_dropdown_label");
    Tether_GUID dd = tether_registry_find_entity("zoo_dropdown");
    if (dd != TETHER_INVALID_GUID) {
        tether_bind_event(dd, TETHER_EVENT_CHANGE, on_dropdown_change, NULL);
    }

    Tether_GUID tabs = tether_registry_find_entity("zoo_switcher_tabs");
    s_switcher = tether_registry_find_entity("zoo_switcher");
    tether_bind_event(tabs, TETHER_EVENT_CHANGE, on_switcher_tab, NULL);
    return root;
}

static bool gallery_text_fits(Tether_GUID entity, const Tether_SlotTransform *bounds) {
    Tether_Visibility *visibility = tether_ecs_get_component(entity, TETHER_COMPONENT_VISIBILITY);
    if (visibility && visibility->computed_state != TETHER_VISIBLE) return true;
    Tether_Text *text = tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT);
    Tether_SlotTransform *slot = tether_ecs_get_component(entity, TETHER_COMPONENT_SLOT_TRANSFORM);
    if (text && slot && text->intrinsic_size.x > 0 &&
        (slot->width <= 0 || text->intrinsic_size.x > slot->width + 1 ||
         slot->x < bounds->x || slot->x + slot->width > bounds->x + bounds->width + 1 ||
         slot->y < bounds->y || slot->y + text->intrinsic_size.y > bounds->y + bounds->height + 1)) {
        fprintf(stderr, "Gallery text does not fit: %s\n", tether_ui_get_text(entity));
        return false;
    }
    Tether_Hierarchy *hierarchy = tether_ecs_get_component(entity, TETHER_COMPONENT_HIERARCHY);
    Tether_GUID child = hierarchy ? hierarchy->first_child : TETHER_INVALID_GUID;
    while (child != TETHER_INVALID_GUID) {
        if (!gallery_text_fits(child, bounds)) return false;
        hierarchy = tether_ecs_get_component(child, TETHER_COMPONENT_HIERARCHY);
        child = hierarchy ? hierarchy->next_sibling : TETHER_INVALID_GUID;
    }
    return true;
}

/* Used by the desktop smoke test after the gallery has been laid out. */
bool sandbox_widgets_zoo_check(void) {
    Tether_GUID toggle = tether_registry_find_entity("zoo_toggle");
    Tether_SlotTransform *toggle_slot = tether_ecs_get_component(toggle, TETHER_COMPONENT_SLOT_TRANSFORM);
    if (!toggle_slot || toggle_slot->width != 46.0f || toggle_slot->height != 26.0f) {
        fprintf(stderr, "Gallery toggle does not preserve its default size\n");
        return false;
    }
    const char *ids[] = {
        "zoo_button_enabled", "zoo_button_disabled", "zoo_checkbox", "zoo_checkbox_disabled",
        "zoo_toggle", "zoo_toggle_disabled", "zoo_slider", "zoo_slider_disabled",
        "zoo_radio", "zoo_radio_disabled",
        "zoo_dropdown", "zoo_dropdown_disabled", "zoo_prog_bar", "zoo_progress_disabled",
        "zoo_col1", "zoo_collapsible_disabled", "zoo_switcher_tabs", "zoo_switcher_tabs_disabled",
        "zoo_switcher", "zoo_switcher_disabled"
    };
    Tether_GUID root = tether_registry_find_entity("zoo_root");
    Tether_SlotTransform *bounds = tether_ecs_get_component(root, TETHER_COMPONENT_SLOT_TRANSFORM);
    if (!bounds || !gallery_text_fits(root, bounds)) return false;
    for (unsigned i = 0; i < sizeof ids / sizeof *ids; ++i) {
        Tether_GUID entity = tether_registry_find_entity(ids[i]);
        Tether_SlotTransform *slot = tether_ecs_get_component(entity, TETHER_COMPONENT_SLOT_TRANSFORM);
        if (!slot || slot->width <= 0 || slot->height <= 0 || slot->x < bounds->x ||
            slot->y < bounds->y || slot->x + slot->width > bounds->x + bounds->width + 1 ||
            slot->y + slot->height > bounds->y + bounds->height + 1) {
            fprintf(stderr, "Gallery control outside its view: %s\n", ids[i]);
            return false;
        }
        if (tether_widget_is_enabled(entity) != ((i % 2) == 0)) {
            fprintf(stderr, "Gallery enabled state mismatch: %s\n", ids[i]);
            return false;
        }
        if (i % 2) {
            Tether_GUID hit = tether_hit_test(slot->x + slot->width * 0.5f,
                                              slot->y + slot->height * 0.5f);
            while (tether_ecs_is_valid(hit)) {
                if (hit == entity) {
                    fprintf(stderr, "Disabled gallery control was hit: %s\n", ids[i]);
                    return false;
                }
                hit = tether_widget_get_parent(hit);
            }
        }
    }

    /* The enabled button responds; the disabled button must not invoke the same callback. */
    Tether_GUID status = tether_registry_find_entity("zoo_button_status");
    for (unsigned i = 0; i < 2; ++i) {
        const char *id = i == 0 ? "zoo_button_disabled" : "zoo_button_enabled";
        Tether_GUID button = tether_registry_find_entity(id);
        Tether_SlotTransform *slot = tether_ecs_get_component(button, TETHER_COMPONENT_SLOT_TRANSFORM);
        Tether_Pointer_Event event = {.type = TETHER_POINTER_DOWN, .button = TETHER_MOUSE_BUTTON_LEFT,
                                      .x = slot->x + slot->width * 0.5f,
                                      .y = slot->y + slot->height * 0.5f};
        tether_input_process_event(&event);
        event.type = TETHER_POINTER_UP;
        tether_input_process_event(&event);
        const char *message = tether_ui_get_text(status);
        const char *expected = i == 0 ? "Try the enabled button." : "Enabled button clicked.";
        if (!message || strcmp(message, expected)) return false;
    }

    Tether_GUID disabled_bar = tether_registry_find_entity("zoo_switcher_tabs_disabled");
    int selected = tether_ui_get_segment_selected(disabled_bar);
    for (int i = 0; i < 3; ++i) {
        Tether_GUID segment = tether_widget_get_child_at(disabled_bar, i);
        Tether_SlotTransform *segment_slot = tether_ecs_get_component(segment, TETHER_COMPONENT_SLOT_TRANSFORM);
        if (!segment_slot) return false;
        Tether_Pointer_Event event = {.type = TETHER_POINTER_DOWN, .button = TETHER_MOUSE_BUTTON_LEFT,
                                      .x = segment_slot->x + segment_slot->width * 0.5f,
                                      .y = segment_slot->y + segment_slot->height * 0.5f};
        tether_input_process_event(&event);
        event.type = TETHER_POINTER_UP;
        tether_input_process_event(&event);
        if (tether_ui_get_segment_selected(disabled_bar) != selected) {
            fprintf(stderr, "Disabled segmented bar changed selection\n");
            return false;
        }
    }

    Tether_GUID slider = tether_registry_find_entity("zoo_slider");
    Tether_GUID thumb = tether_widget_get_child_at(slider, 1);
    Tether_SlotTransform *slot = tether_ecs_get_component(thumb, TETHER_COMPONENT_SLOT_TRANSFORM);
    if (!slot) return false;
    Tether_Pointer_Event hover = {.type = TETHER_POINTER_MOVE, .button = TETHER_MOUSE_BUTTON_NONE,
                                   .x = slot->x + slot->width * 0.5f,
                                   .y = slot->y + slot->height * 0.5f};
    tether_input_process_event(&hover);
    Tether_Shape *shape = tether_ecs_get_component(thumb, TETHER_COMPONENT_SHAPE);
    if (!shape || shape->active_bg.r <= shape->bg_color.r) {
        fprintf(stderr, "Slider thumb did not brighten on hover\n");
        return false;
    }
    return true;
}
