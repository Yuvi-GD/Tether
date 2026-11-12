#include "tether/tether.h"
#include "tether/backends/tether_backend.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_input.h"
#include "tether/core/tether_registry.h"
#include "tether/engine/tether_engine.h"
#include "tether/engine/tether_layout.h"
#include "tether/engine/parser/tether_yaml_ast.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "tether/ui/tether_list.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* These cases exercise CPU-side contracts without opening a graphics device.
   They do not stand in for font rasterization or pixel-comparison tests. */
static unsigned checks;

static bool test_resource_alive(uint32_t handle) {
    (void)handle;
    return false;
}

static uint32_t test_resource_create(void) {
    return 0;
}

static void test_resolution(uint32_t *width, uint32_t *height) {
    *width = 800;
    *height = 600;
}

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        checks++;                                                                                  \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);                        \
            return false;                                                                          \
        }                                                                                          \
    } while (0)

static bool near(float actual, float expected) {
    return fabsf(actual - expected) < 0.01f;
}

static Tether_GUID panel(Tether_GUID parent, float width, float height) {
    Tether_GUID entity = tether_ui_panel(parent);
    tether_ui_set_size(entity, width, height);
    return entity;
}

static bool layout_case(void) {
    Tether_GUID root = panel(tether_ecs_get_main_root(), 400, 200);
    Tether_Layout *layout = tether_ecs_get_component(root, TETHER_COMPONENT_LAYOUT);
    layout->flow = TETHER_FLOW_ROW;
    layout->content_align_x = TETHER_ALIGN_CENTER;
    layout->content_align_y = TETHER_ALIGN_START;
    tether_ui_set_gap(root, 10, 0);
    Tether_GUID first = panel(root, 50, 20);
    Tether_GUID second = panel(root, 50, 20);
    tether_layout_process_all(400, 200);
    Tether_SlotTransform *a = tether_ecs_get_component(first, TETHER_COMPONENT_SLOT_TRANSFORM);
    Tether_SlotTransform *b = tether_ecs_get_component(second, TETHER_COMPONENT_SLOT_TRANSFORM);
    CHECK(a && b);
    CHECK(near(a->x, 145) && near(b->x, 205));
    CHECK(near(a->width, 50) && near(a->height, 20));

    layout = tether_ecs_get_component(root, TETHER_COMPONENT_LAYOUT);
    layout->content_align_x = TETHER_ALIGN_END;
    tether_layout_process_all(400, 200);
    CHECK(near(a->x, 290) && near(b->x, 350));
    tether_ui_set_size(root, 600, 200);
    tether_layout_process_all(600, 200);
    CHECK(near(a->x, 490) && near(b->x, 550));

    layout = tether_ecs_get_component(root, TETHER_COMPONENT_LAYOUT);
    layout->flow = TETHER_FLOW_COLUMN;
    layout->content_align_y = TETHER_ALIGN_CENTER;
    layout->content_align_x = TETHER_ALIGN_START;
    tether_ui_set_gap(root, 0, 10);
    tether_layout_process_all(600, 200);
    CHECK(near(a->y, 75) && near(b->y, 105));
    tether_ui_set_content_align(root, TETHER_ALIGN_FILL, TETHER_ALIGN_START);
    Tether_GUID toggle = tether_ui_toggle(root);
    tether_layout_process_all(600, 200);
    Tether_SlotTransform *toggle_slot = tether_ecs_get_component(toggle, TETHER_COMPONENT_SLOT_TRANSFORM);
    CHECK(toggle_slot && near(toggle_slot->width, 46) && near(toggle_slot->height, 26));
    CHECK(tether_ecs_get_component(root, TETHER_COMPONENT_DIRTY_LAYOUT) != NULL);
    return true;
}

static bool visibility_case(void) {
    Tether_GUID parent = panel(tether_ecs_get_main_root(), 300, 100);
    Tether_GUID child = panel(parent, 20, 20);
    Tether_Visibility *visibility = tether_ecs_get_component(child, TETHER_COMPONENT_VISIBILITY);
    CHECK(visibility && visibility->computed_state == TETHER_VISIBLE);
    tether_widget_set_visibility(parent, TETHER_HIDDEN);
    CHECK(visibility->computed_state == TETHER_HIDDEN);
    CHECK(visibility->local_state == TETHER_VISIBLE);
    tether_widget_set_visibility(child, TETHER_COLLAPSED);
    tether_widget_set_visibility(parent, TETHER_VISIBLE);
    CHECK(visibility->computed_state == TETHER_COLLAPSED);
    tether_widget_set_visibility(child, TETHER_VISIBLE);
    CHECK(visibility->computed_state == TETHER_VISIBLE);
    CHECK(tether_ecs_get_component(child, TETHER_COMPONENT_DIRTY_VISUAL) != NULL);
    CHECK(tether_ecs_get_component(child, TETHER_COMPONENT_DIRTY_LAYOUT) != NULL);
    return true;
}

static void count_click(Tether_GUID entity, const Tether_Event *event_info, void *data) {
    (void)entity;
    if (event_info->type == TETHER_EVENT_CLICK) {
        (*(unsigned *)data)++;
    }
}

static bool input_case(void) {
    Tether_GUID root = panel(tether_ecs_get_main_root(), 200, 100);
    Tether_GUID lower = panel(root, 100, 100);
    Tether_GUID upper = panel(root, 100, 100);
    tether_ecs_add_component(lower, TETHER_COMPONENT_INTERACTABLE);
    tether_ecs_add_component(upper, TETHER_COMPONENT_INTERACTABLE);
    tether_layout_process_all(200, 100);
    CHECK(tether_hit_test(10, 10) == upper);
    tether_widget_set_visibility(upper, TETHER_HIDDEN);
    CHECK(tether_hit_test(10, 10) == lower);
    tether_widget_set_visibility(upper, TETHER_VISIBLE);
    Tether_Interactable *input = tether_ecs_get_component(upper, TETHER_COMPONENT_INTERACTABLE);
    CHECK(input != NULL);
    input->hit_behavior = TETHER_HIT_IGNORE;
    CHECK(tether_hit_test(10, 10) == lower);
    unsigned clicks = 0;
    tether_bind_event(lower, TETHER_EVENT_CLICK, count_click, &clicks);
    Tether_Pointer_Event event = {TETHER_POINTER_DOWN, TETHER_MOUSE_BUTTON_LEFT, 10, 10};
    tether_input_process_event(&event);
    event.type = TETHER_POINTER_UP;
    tether_input_process_event(&event);
    CHECK(clicks == 1);
    tether_unbind_event(lower, TETHER_EVENT_CLICK);
    tether_dispatch_event(lower, TETHER_EVENT_CLICK);
    CHECK(clicks == 1);
    tether_input_process_event(NULL);
    CHECK(tether_hit_test(-1, -1) == TETHER_INVALID_GUID);
    return true;
}

static bool lifetime_case(void) {
    Tether_GUID parent = panel(tether_ecs_get_main_root(), 100, 100);
    Tether_GUID child = panel(parent, 10, 10);
    tether_ecs_destroy_entity(parent);
    CHECK(!tether_ecs_is_valid(parent));
    CHECK(!tether_ecs_is_valid(child));
    tether_ecs_destroy_entity(parent);
    tether_ecs_destroy_entity(TETHER_INVALID_GUID);
    Tether_GUID replacement = tether_ecs_create_entity();
    CHECK(tether_ecs_is_valid(replacement));
    CHECK(replacement != parent && replacement != child);
    CHECK(tether_ecs_get_component(child, TETHER_COMPONENT_LAYOUT) == NULL);
    CHECK(!tether_ui_set_text(child, "stale"));
    return true;
}

static bool text_case(void) {
    Tether_GUID text = tether_ui_text(tether_ecs_get_main_root());
    char large[700];
    memset(large, 'x', sizeof large - 1);
    large[sizeof large - 1] = '\0';
    const char *values[] = {"word", "A label longer than thirty-one characters", large, "",
                            "again"};
    for (unsigned i = 0; i < sizeof values / sizeof *values; i++) {
        CHECK(tether_ui_set_text(text, values[i]));
        CHECK(!strcmp(tether_ui_get_text(text), values[i]));
        CHECK(tether_ecs_get_component(text, TETHER_COMPONENT_DIRTY_TEXT_CONTENT) != NULL);
    }
    tether_ecs_remove_component(text, TETHER_COMPONENT_DIRTY_LAYOUT);
    tether_ecs_remove_component(text, TETHER_COMPONENT_DIRTY_VISUAL);
    tether_ui_set_text_align(text, TETHER_ALIGN_CENTER, TETHER_ALIGN_END);
    Tether_Text *style = tether_ecs_get_component(text, TETHER_COMPONENT_TEXT);
    CHECK(style && style->align_x == TETHER_ALIGN_CENTER && style->align_y == TETHER_ALIGN_END);
    CHECK(tether_ecs_get_component(text, TETHER_COMPONENT_DIRTY_LAYOUT) != NULL);
    CHECK(tether_ecs_get_component(text, TETHER_COMPONENT_DIRTY_VISUAL) != NULL);
    CHECK(tether_ecs_get_component(text, TETHER_COMPONENT_VOLATILE) == NULL);
    return true;
}

static int s_test_ctx_selected_idx = -1;
static unsigned s_test_ctx_actions;
static char s_test_ctx_selected_str[64] = {0};
static void test_on_ctx_select(Tether_GUID item, const Tether_Event *event_info, void *user_data) {
    (void)item;
    (void)event_info;
    Tether_GUID menu = (Tether_GUID)(uintptr_t)user_data;
    s_test_ctx_actions++;
    int index = tether_ui_menu_get_selected(menu);
    const char *item_text = tether_ui_menu_get_item_text(menu, index);
    s_test_ctx_selected_idx = index;
    snprintf(s_test_ctx_selected_str, sizeof(s_test_ctx_selected_str), "%s", item_text ? item_text : "");
}

static bool widgets_case(void) {
    Tether_GUID root = tether_ecs_get_main_root();

    /* 0. Button & Hierarchy Queries */
    unsigned btn_clicks = 0;
    Tether_GUID btn = tether_ui_button(root);
    CHECK(tether_ecs_is_valid(btn));
    CHECK(tether_widget_get_parent(btn) == root);
    Tether_GUID btn_txt = tether_widget_get_child_at(btn, 0);
    CHECK(tether_ecs_is_valid(btn_txt));
    CHECK(tether_widget_get_child_at(btn, 0) == btn_txt);
    CHECK(tether_widget_get_child_at(btn, -1) == btn_txt);
    CHECK(tether_widget_get_child_at(btn, 1) == TETHER_INVALID_GUID);
    int btn_child_count = tether_widget_get_child_count(btn);
    CHECK(btn_child_count == 1);
    CHECK(tether_widget_get_child_count(btn) == 1);
    CHECK(strcmp(tether_ui_get_text(btn_txt), "Button") == 0);
    tether_ui_set_text(btn_txt, "Click Me");
    CHECK(strcmp(tether_ui_get_text(btn_txt), "Click Me") == 0);
    tether_ui_set_color(btn, (Tether_Color){255, 0, 0, 255});
    tether_bind_event(btn, TETHER_EVENT_CLICK, count_click, &btn_clicks);
    tether_dispatch_event(btn, TETHER_EVENT_CLICK);
    CHECK(btn_clicks == 1);

    /* 1. Progress Bar */
    Tether_GUID pb = tether_ui_progress_bar(root);
    tether_ui_set_size(pb, 200.0f, 10.0f);
    tether_ui_set_color(pb, (Tether_Color){30, 30, 30, 255});
    tether_ui_set_progress(pb, 0.25f);
    CHECK(tether_ecs_is_valid(pb));
    CHECK(near(tether_ui_get_progress(pb), 0.25f));
    CHECK(near(tether_ui_get_value(pb), 0.25f));
    tether_ui_set_progress(pb, 0.85f);
    CHECK(near(tether_ui_get_progress(pb), 0.85f));
    tether_ui_set_progress(pb, 1.5f);
    CHECK(near(tether_ui_get_progress(pb), 1.0f));
    tether_ui_set_progress(pb, -0.5f);
    CHECK(near(tether_ui_get_progress(pb), 0.0f));

    /* 2. Slider */
    Tether_GUID sl = tether_ui_slider(root);
    tether_ui_set_size(sl, 200.0f, 24.0f);
    tether_ui_set_slider_range(sl, 0.0f, 100.0f);
    tether_ui_set_slider_value(sl, 50.0f);
    CHECK(tether_ecs_is_valid(sl));
    CHECK(near(tether_ui_get_slider_value(sl), 50.0f));
    CHECK(near(tether_ui_get_value(sl), 50.0f));
    tether_ui_set_slider_value(sl, 75.0f);
    CHECK(near(tether_ui_get_slider_value(sl), 75.0f));
    CHECK(near(tether_ui_get_value(sl), 75.0f));
    tether_ui_set_slider_value(sl, 120.0f);
    CHECK(near(tether_ui_get_slider_value(sl), 100.0f));
    CHECK(near(tether_ui_get_value(sl), 100.0f));
    tether_ui_set_value(sl, 30.0f);
    CHECK(near(tether_ui_get_value(sl), 30.0f));
    CHECK(near(tether_ui_get_slider_value(sl), 30.0f));

    /* 3. Toggle Switch */
    Tether_GUID tg = tether_ui_toggle(root);
    tether_ui_set_toggle(tg, false);
    CHECK(tether_ecs_is_valid(tg));
    CHECK(!tether_ui_get_toggle(tg));
    tether_ui_set_toggle(tg, true);
    CHECK(tether_ui_get_toggle(tg));
    tether_ui_set_toggle(tg, false);
    CHECK(!tether_ui_get_toggle(tg));

    /* 4. Checkbox */
    Tether_GUID cb = tether_ui_checkbox(root);
    CHECK(tether_ecs_is_valid(cb));
    CHECK(!tether_ui_get_checked(cb));
    tether_ui_set_checked(cb, true);
    CHECK(tether_ui_get_checked(cb));
    tether_ui_set_checked(cb, false);
    CHECK(!tether_ui_get_checked(cb));

    /* 5. Radio Button Group */
    Tether_GUID rad = tether_ui_radio(root);
    CHECK(tether_ecs_is_valid(rad));
    CHECK(tether_ui_get_radio_selected(rad) == -1);
    tether_ui_add_radio_option(rad, "Option 1");
    tether_ui_add_radio_option(rad, "Option 2");
    CHECK(tether_list_get_count(rad) == 2);
    CHECK(strcmp(tether_ui_get_radio_option(rad, 0), "Option 1") == 0);
    CHECK(strcmp(tether_ui_get_radio_option(rad, 1), "Option 2") == 0);
    tether_ui_set_radio_selected(rad, 0);
    CHECK(tether_ui_get_radio_selected(rad) == 0);
    tether_ui_set_radio_selected(rad, 1);
    CHECK(tether_ui_get_radio_selected(rad) == 1);
    tether_ui_unselect_radio(rad);
    CHECK(tether_ui_get_radio_selected(rad) == -1);

    /* 6. Segmented Button Bar */
    Tether_GUID bar = tether_ui_segmented_bar(root);
    CHECK(tether_ecs_is_valid(bar));
    CHECK(tether_ui_get_segment_selected(bar) == -1);
    tether_ui_add_segment(bar, "Option A");
    tether_ui_add_segment(bar, "Option B");
    tether_ui_add_segment(bar, "Option C");
    CHECK(tether_list_get_count(bar) == 3);
    tether_ui_set_segment_selected(bar, 2);
    CHECK(tether_ui_get_segment_selected(bar) == 2);
    tether_ui_unselect_segment(bar);
    CHECK(tether_ui_get_segment_selected(bar) == -1);

    /* 7. Dropdown (Inline & Overlay) */
    const char *dd_opts[] = {"Red", "Green", "Blue"};
    Tether_GUID dd = tether_ui_dropdown(root);
    tether_ui_set_dropdown_options(dd, dd_opts, 3);
    tether_ui_set_dropdown_selected(dd, 0);
    tether_ui_set_dropdown_type(dd, TETHER_DROPDOWN_INLINE);
    CHECK(tether_ecs_is_valid(dd));
    CHECK(tether_ui_get_dropdown_selected(dd) == 0);

    /* Verify bracket chevron and RenderTransform scale & rotation */
    Tether_GUID dd_hdr = tether_widget_get_child_at(dd, 0);
    Tether_GUID dd_arrow = tether_widget_get_child_at(dd_hdr, 1);
    CHECK(tether_ecs_is_valid(dd_arrow));
    CHECK(strcmp(tether_ui_get_text(dd_arrow), "<") == 0);
    Tether_RenderTransform *dd_arrow_rt = (Tether_RenderTransform*)tether_ecs_get_component(dd_arrow, TETHER_COMPONENT_RENDER_TRANSFORM);
    CHECK(dd_arrow_rt != NULL);
    CHECK(dd_arrow_rt->scale_x == 1.0f);
    CHECK(dd_arrow_rt->scale_y == 1.0f);
    CHECK(dd_arrow_rt->rotation_deg == 180.0f);

    tether_ui_set_dropdown_selected(dd, 1);
    CHECK(tether_ui_get_dropdown_selected(dd) == 1);
    Tether_GUID dd_label = tether_widget_get_child_at(dd_hdr, 0);
    CHECK(strcmp(tether_ui_get_text(dd_label), "Green") == 0);

    CHECK(!tether_ui_is_dropdown_open(dd));
    tether_ui_set_dropdown_open(dd, true);
    CHECK(tether_ui_is_dropdown_open(dd));
    CHECK(dd_arrow_rt->rotation_deg == -90.0f);
    tether_ui_set_dropdown_open(dd, false);
    CHECK(!tether_ui_is_dropdown_open(dd));
    CHECK(dd_arrow_rt->rotation_deg == 180.0f);

    /* Test Header Click: press and release on header must leave dropdown open */
    tether_layout_process_all(800.0f, 600.0f);
    Tether_SlotTransform *hdr_st = (Tether_SlotTransform*)tether_ecs_get_component(dd_hdr, TETHER_COMPONENT_SLOT_TRANSFORM);
    if (hdr_st && hdr_st->width > 0.0f) {
        float cx = hdr_st->x + 10.0f;
        float cy = hdr_st->y + 10.0f;
        Tether_Pointer_Event pe_hdr_down = { .type = TETHER_POINTER_DOWN, .button = TETHER_MOUSE_BUTTON_LEFT, .x = cx, .y = cy };
        tether_input_process_event(&pe_hdr_down);
        CHECK(tether_ui_is_dropdown_open(dd));
        Tether_Pointer_Event pe_hdr_up = { .type = TETHER_POINTER_UP, .button = TETHER_MOUSE_BUTTON_LEFT, .x = cx, .y = cy };
        tether_input_process_event(&pe_hdr_up);
        /* Must stay open after releasing mouse on header */
        CHECK(tether_ui_is_dropdown_open(dd));
    }

    /* Test Outside Click Collapse */
    tether_ui_set_dropdown_open(dd, true);
    CHECK(tether_ui_is_dropdown_open(dd));
    Tether_Pointer_Event pe_outside_down = {
        .type = TETHER_POINTER_DOWN,
        .button = TETHER_MOUSE_BUTTON_LEFT,
        .x = 9999.0f,
        .y = 9999.0f
    };
    tether_input_process_event(&pe_outside_down);
    CHECK(!tether_ui_is_dropdown_open(dd));

    /* Test Drag Outside and Release */
    tether_ui_set_dropdown_open(dd, true);
    CHECK(tether_ui_is_dropdown_open(dd));
    Tether_Pointer_Event pe_move_out = {
        .type = TETHER_POINTER_MOVE,
        .button = TETHER_MOUSE_BUTTON_NONE,
        .x = 9999.0f,
        .y = 9999.0f
    };
    tether_input_process_event(&pe_move_out);
    /* Dragging outside must NOT collapse while mouse is still down */
    CHECK(tether_ui_is_dropdown_open(dd));
    Tether_Pointer_Event pe_outside_up = {
        .type = TETHER_POINTER_UP,
        .button = TETHER_MOUSE_BUTTON_LEFT,
        .x = 9999.0f,
        .y = 9999.0f
    };
    tether_input_process_event(&pe_outside_up);
    /* Releasing outside collapses */
    CHECK(!tether_ui_is_dropdown_open(dd));

    Tether_GUID dd_overlay = tether_ui_dropdown(root);
    tether_ui_set_dropdown_options(dd_overlay, dd_opts, 3);
    tether_ui_set_dropdown_selected(dd_overlay, 0);
    tether_ui_set_dropdown_type(dd_overlay, TETHER_DROPDOWN_OVERLAY);
    CHECK(tether_ecs_is_valid(dd_overlay));
    tether_ui_set_dropdown_open(dd_overlay, true);
    CHECK(tether_ui_is_dropdown_open(dd_overlay));
    tether_ui_set_dropdown_open(dd_overlay, false);
    CHECK(!tether_ui_is_dropdown_open(dd_overlay));

    /* Context menus are created and configured once, then reopened. */
    Tether_GUID ctx_host = tether_widget_create_container(tether_ecs_get_overlay_root());
    Tether_System_API *original_system = g_backend.system;
    Tether_System_API test_system = *original_system;
    test_system.get_resolution = test_resolution;
    g_backend.system = &test_system;
    tether_ui_set_container(ctx_host, TETHER_FLOW_NONE);
    Tether_SlotTransform *ctx_host_slot = tether_ecs_get_component(ctx_host, TETHER_COMPONENT_SLOT_TRANSFORM);
    ctx_host_slot->width = 800.0f;
    ctx_host_slot->height = 600.0f;
    Tether_GUID ctx_menu2 = tether_ui_context_menu(ctx_host);
    tether_ui_menu_set_items(ctx_menu2, dd_opts, 3);
    for (int i = 0; i < 3; ++i) {
        tether_bind_event(tether_widget_get_child_at(ctx_menu2, i), TETHER_EVENT_CLICK,
                          test_on_ctx_select, (void *)(uintptr_t)ctx_menu2);
    }
    tether_ui_context_menu_open_at(ctx_menu2, 150.0f, 250.0f);
    CHECK(tether_ui_menu_is_open(ctx_menu2));
    CHECK(s_test_ctx_selected_idx == -1);
    CHECK(s_test_ctx_actions == 0);
    tether_layout_process_all(800.0f, 600.0f);
    Tether_GUID btn1 = tether_widget_get_child_at(ctx_menu2, 1);
    CHECK(tether_ecs_is_valid(btn1));
    Tether_SlotTransform *b1_st = (Tether_SlotTransform*)tether_ecs_get_component(btn1, TETHER_COMPONENT_SLOT_TRANSFORM);
    if (b1_st) {
        float bx = b1_st->x + 5.0f;
        float by = b1_st->y + 5.0f;
        Tether_Pointer_Event pe_down = { .type = TETHER_POINTER_DOWN, .button = TETHER_MOUSE_BUTTON_LEFT, .x = bx, .y = by };
        tether_input_process_event(&pe_down);
        Tether_Pointer_Event pe_up = { .type = TETHER_POINTER_UP, .button = TETHER_MOUSE_BUTTON_LEFT, .x = bx, .y = by };
        tether_input_process_event(&pe_up);
        CHECK(s_test_ctx_selected_idx == 1);
        CHECK(s_test_ctx_actions == 1);
        CHECK(strcmp(s_test_ctx_selected_str, "Green") == 0);
        CHECK(!tether_ui_menu_is_open(ctx_menu2));
    }

    /* Context menu outside click dismiss test */
    tether_ui_context_menu_open_at(ctx_menu2, 150.0f, 250.0f);
    CHECK(tether_ui_menu_is_open(ctx_menu2));
    Tether_Pointer_Event pe_out = { .type = TETHER_POINTER_DOWN, .button = TETHER_MOUSE_BUTTON_LEFT, .x = 9999.0f, .y = 9999.0f };
    tether_input_process_event(&pe_out);
    CHECK(!tether_ui_menu_is_open(ctx_menu2));
    CHECK(tether_ecs_is_valid(ctx_menu2));
    tether_ui_context_menu_open_at(ctx_menu2, 799.0f, 599.0f);
    tether_layout_process_all(800.0f, 600.0f);
    Tether_SlotTransform *ctx_slot = tether_ecs_get_component(ctx_menu2, TETHER_COMPONENT_SLOT_TRANSFORM);
    CHECK(ctx_slot && ctx_slot->x >= 0 && ctx_slot->y >= 0);
    CHECK(ctx_slot->x + ctx_slot->width <= 800.0f);
    CHECK(ctx_slot->y + ctx_slot->height <= 600.0f);
    tether_ui_menu_close(ctx_menu2);
    CHECK(s_test_ctx_actions == 1);
    g_backend.system = original_system;

    /* 8. Collapsible Section */
    Tether_GUID col = tether_ui_collapsible(root);
    tether_ui_set_collapsible_title(col, "Telemetry Details");
    CHECK(tether_ecs_is_valid(col));
    CHECK(!tether_ui_is_collapsible_open(col));
    Tether_GUID content = tether_ui_collapsible_content(col);
    CHECK(tether_ecs_is_valid(content));
    CHECK(tether_widget_is_collapsed(content));
    tether_ui_toggle_collapsible(col);
    CHECK(tether_ui_is_collapsible_open(col));
    CHECK(tether_widget_is_visible(content));

    /* 8. View Switcher */
    Tether_GUID sw = tether_ui_switcher(root);
    CHECK(tether_ecs_is_valid(sw));
    Tether_GUID v0 = tether_ui_switcher_add_view(sw);
    Tether_GUID v1 = tether_ui_switcher_add_view(sw);
    Tether_GUID v2 = tether_ui_switcher_add_view(sw);
    CHECK(tether_ui_switcher_get_active(sw) == 0);
    CHECK(tether_widget_is_visible(v0));
    CHECK(tether_widget_is_collapsed(v1));
    CHECK(tether_widget_is_collapsed(v2));

    tether_ui_switcher_show_index(sw, 1);
    CHECK(tether_ui_switcher_get_active(sw) == 1);
    CHECK(tether_widget_is_collapsed(v0));
    CHECK(tether_widget_is_visible(v1));
    CHECK(tether_widget_is_collapsed(v2));

    return true;
}

static void count_change(Tether_GUID entity, const Tether_Event *event_info, void *data) {
    (void)entity;
    if (event_info->type == TETHER_EVENT_CHANGE) (*(unsigned *)data)++;
}

static bool menu_interaction_case(void) {
    Tether_GUID root = panel(tether_ecs_get_main_root(), 400, 300);
    tether_ui_set_container(root, TETHER_FLOW_COLUMN);
    Tether_GUID dropdown = tether_ui_dropdown(root);
    const char *items[] = {"First", "Second", "Third"};
    tether_ui_set_dropdown_options(dropdown, items, 3);
    /* A later sibling covers the menu's lower items in normal layout order. */
    Tether_GUID overlapping = panel(root, 400, 140);
    tether_widget_add_interaction(overlapping, TETHER_HIT_BLOCK);
    Tether_GUID header = tether_widget_get_child_at(dropdown, 0);
    Tether_GUID menu = tether_widget_get_child_at(dropdown, 1);
    Tether_Shape *frame = tether_ecs_get_component(menu, TETHER_COMPONENT_SHAPE);
    Tether_Shape *item_shape = tether_ecs_get_component(tether_widget_get_child_at(menu, 0), TETHER_COMPONENT_SHAPE);
    CHECK(frame && item_shape && frame->border_width > 0.0f);
    CHECK(memcmp(&frame->bg_color, &item_shape->bg_color, sizeof(Tether_Color)) != 0);

    unsigned changes = 0;
    tether_bind_event(dropdown, TETHER_EVENT_CHANGE, count_change, &changes);
    tether_layout_process_all(400, 300);
    Tether_SlotTransform *header_slot = tether_ecs_get_component(header, TETHER_COMPONENT_SLOT_TRANSFORM);
    CHECK(header_slot && header_slot->width > 0 && header_slot->height > 0);
    Tether_Pointer_Event event = { .type = TETHER_POINTER_DOWN, .button = TETHER_MOUSE_BUTTON_LEFT,
                                   .x = header_slot->x + 5, .y = header_slot->y + 5 };
    CHECK(tether_hit_test(event.x, event.y) == header);
    tether_input_process_event(&event);
    CHECK(tether_ui_is_dropdown_open(dropdown));
    tether_layout_process_all(400, 300);
    Tether_GUID last = tether_widget_get_child_at(menu, 2);
    unsigned actions = 0;
    tether_bind_event(last, TETHER_EVENT_CLICK, count_click, &actions);
    Tether_SlotTransform *last_slot = tether_ecs_get_component(last, TETHER_COMPONENT_SLOT_TRANSFORM);
    CHECK(last_slot && last_slot->width > 0 && last_slot->height > 0);
    event.type = TETHER_POINTER_MOVE;
    event.button = TETHER_MOUSE_BUTTON_NONE;
    event.x = last_slot->x + 5;
    event.y = last_slot->y + 5;
    CHECK(tether_hit_test(event.x, event.y) == last);
    tether_input_process_event(&event);
    Tether_Interactable *last_interaction = tether_ecs_get_component(last, TETHER_COMPONENT_INTERACTABLE);
    CHECK(last_interaction && last_interaction->is_hovered);
    event.type = TETHER_POINTER_UP;
    event.button = TETHER_MOUSE_BUTTON_LEFT;
    tether_input_process_event(&event);
    CHECK(tether_ui_get_dropdown_selected(dropdown) == 2);
    CHECK(changes == 3); /* Open, selection, close each change observable state. */
    CHECK(actions == 1);
    CHECK(!tether_ui_is_dropdown_open(dropdown));

    Tether_GUID collapsible = tether_ui_collapsible(root);
    Tether_GUID col_header = tether_widget_get_child_at(collapsible, 0);
    Tether_GUID col_arrow = tether_widget_get_child_at(col_header, 0);
    Tether_GUID dd_arrow = tether_widget_get_child_at(header, 1);
    Tether_Text *col_text = tether_ecs_get_component(col_arrow, TETHER_COMPONENT_TEXT);
    Tether_Text *dd_text = tether_ecs_get_component(dd_arrow, TETHER_COMPONENT_TEXT);
    Tether_RenderTransform *col_transform = tether_ecs_get_component(col_arrow, TETHER_COMPONENT_RENDER_TRANSFORM);
    CHECK(col_text && dd_text && col_text->font_size == dd_text->font_size);
    CHECK(strcmp(tether_ui_get_text(col_arrow), "<") == 0);
    CHECK(col_transform && col_transform->rotation_deg == 180.0f);
    tether_ui_toggle_collapsible(collapsible);
    CHECK(col_transform->rotation_deg == -90.0f);
    Tether_Shape *col_shape = tether_ecs_get_component(col_header, TETHER_COMPONENT_SHAPE);
    Tether_Interactable *interaction = tether_ecs_get_component(col_header, TETHER_COMPONENT_INTERACTABLE);
    CHECK(col_shape && interaction);
    interaction->is_hovered = 1;
    tether_dispatch_event(col_header, TETHER_EVENT_HOVER_ENTER);
    CHECK(memcmp(&col_shape->active_bg, &col_shape->bg_color, sizeof(Tether_Color)) != 0);
    interaction->is_hovered = 0;
    tether_dispatch_event(col_header, TETHER_EVENT_HOVER_EXIT);
    CHECK(memcmp(&col_shape->active_bg, &col_shape->bg_color, sizeof(Tether_Color)) == 0);
    return true;
}

static unsigned event_self_count, event_other_count, event_right_count;
static unsigned event_change_count;
static void event_self_remove(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)event;
    (void)data;
    event_self_count++;
    tether_unbind_callback(entity, TETHER_EVENT_CLICK, event_self_remove);
}

static void event_other(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity;
    (void)event;
    (void)data;
    event_other_count++;
}

static void event_right(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity;
    (void)data;
    if (event->type == TETHER_EVENT_CLICK && event->button == TETHER_MOUSE_BUTTON_RIGHT &&
        near(event->x, 10.0f) && near(event->y, 10.0f)) event_right_count++;
}

static void event_menu_change(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity;
    (void)event;
    (void)data;
    event_change_count++;
}

static bool event_contract_case(void) {
    Tether_GUID root = panel(tether_ecs_get_main_root(), 200, 100);
    Tether_GUID child = panel(root, 100, 100);
    tether_widget_add_interaction(child, TETHER_HIT_BLOCK);
    tether_bind_event(child, TETHER_EVENT_CLICK, event_self_remove, NULL);
    tether_bind_event(child, TETHER_EVENT_CLICK, event_other, NULL);
    tether_dispatch_event(child, TETHER_EVENT_CLICK);
    CHECK(event_self_count == 1 && event_other_count == 1);
    tether_dispatch_event(child, TETHER_EVENT_CLICK);
    CHECK(event_self_count == 1 && event_other_count == 2);

    tether_bind_pointer_event(tether_ecs_get_main_root(), TETHER_EVENT_CLICK,
                              TETHER_MOUSE_BUTTON_RIGHT, event_right, NULL);
    tether_layout_process_all(200, 100);
    Tether_Pointer_Event pointer = { .type = TETHER_POINTER_DOWN, .button = TETHER_MOUSE_BUTTON_RIGHT,
                                     .x = 10, .y = 10 };
    tether_input_process_event(&pointer);
    CHECK(event_right_count == 0);
    pointer.type = TETHER_POINTER_UP;
    tether_input_process_event(&pointer);
    CHECK(event_right_count == 1);
    CHECK(event_other_count == 2); /* Default left binding ignored the right click. */

    Tether_GUID menu = tether_ui_menu(root);
    const char *items[] = {"One", "Two"};
    tether_ui_menu_set_items(menu, items, 2);
    tether_bind_event(menu, TETHER_EVENT_CHANGE, event_menu_change, NULL);
    tether_ui_menu_open(menu);
    CHECK(event_change_count == 1);
    tether_input_term();
    tether_ui_menu_open(menu);
    tether_ui_menu_close(menu);
    CHECK(event_change_count == 2);
    return true;
}

static unsigned disabled_click_count;
static void count_disabled_click(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity;
    (void)event;
    (void)data;
    ++disabled_click_count;
}

/* Check the slider after disabling its parent during a drag. */
static bool disabled_case(void) {
    Tether_GUID root = panel(tether_ecs_get_main_root(), 200, 100);
    Tether_GUID slider = tether_ui_slider(root);
    tether_ui_set_size(slider, 100, 20);
    Tether_ValueRange *range = tether_ecs_get_component(slider, TETHER_COMPONENT_VALUE_RANGE);
    CHECK(range != NULL);
    range->step = 10.0f;
    tether_ui_set_slider_value(slider, 26.0f);
    CHECK(tether_ui_get_slider_value(slider) == 30.0f);
    Tether_GUID thumb = tether_widget_get_child_at(slider, 1);
    Tether_RenderTransform *transform = tether_ecs_get_component(thumb, TETHER_COMPONENT_RENDER_TRANSFORM);
    CHECK(transform != NULL);
    transform->opacity = 0.8f;
    tether_layout_process_all(200, 100);
    Tether_SlotTransform *position = tether_ecs_get_component(slider, TETHER_COMPONENT_SLOT_TRANSFORM);
    CHECK(position && position->width > 20);
    Tether_Pointer_Event event = {.type = TETHER_POINTER_DOWN, .button = TETHER_MOUSE_BUTTON_LEFT,
                                  .x = position->x + 25, .y = position->y + 5};
    tether_input_process_event(&event);
    float value = tether_ui_get_slider_value(slider);
    tether_widget_set_enabled(root, false);
    CHECK(!tether_widget_is_enabled(root));
    /* The child retains its own enabled flag; the parent blocks hit testing. */
    CHECK(tether_widget_is_enabled(slider));
    CHECK(near(transform->opacity, 0.4f));
    event.type = TETHER_POINTER_MOVE;
    event.x = position->x + 80;
    tether_input_process_event(&event);
    CHECK(tether_ui_get_slider_value(slider) == value);
    tether_widget_set_enabled(root, true);
    CHECK(near(transform->opacity, 0.8f));
    tether_input_process_event(&event);
    Tether_Interactable *input = tether_ecs_get_component(slider, TETHER_COMPONENT_INTERACTABLE);
    CHECK(input && input->is_hovered);
    CHECK(tether_ui_get_slider_value(slider) == value);
    event.type = TETHER_POINTER_UP;
    tether_input_process_event(&event);
    tether_ui_set_interactable(slider, TETHER_HIT_SELF_ONLY);
    tether_widget_set_enabled(slider, false);
    tether_widget_set_enabled(root, false);
    tether_widget_set_enabled(root, true);
    CHECK(!tether_widget_is_enabled(slider));
    CHECK(near(transform->opacity, 0.4f));
    tether_widget_set_enabled(slider, true);
    CHECK(tether_widget_is_enabled(slider));
    CHECK(tether_ui_get_interactable(slider) == TETHER_HIT_SELF_ONLY);
    CHECK(near(transform->opacity, 0.8f));

    tether_widget_destroy(slider);
    Tether_GUID button = tether_ui_button(root);
    tether_ui_set_size(button, 100, 30);
    tether_bind_event(button, TETHER_EVENT_CLICK, count_disabled_click, NULL);
    tether_layout_process_all(200, 100);
    position = tether_ecs_get_component(button, TETHER_COMPONENT_SLOT_TRANSFORM);
    CHECK(position != NULL);
    event.x = position->x + 5;
    event.y = position->y + 5;
    event.type = TETHER_POINTER_DOWN;
    tether_input_process_event(&event);
    tether_widget_set_enabled(root, false);
    tether_widget_set_enabled(root, true);
    event.type = TETHER_POINTER_UP;
    tether_input_process_event(&event);
    CHECK(disabled_click_count == 0);
    return true;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: tether_behavior_test layout|visibility|input|lifetime|text|widgets|menu_interaction\n");
        return 2;
    }
    tether_init(NULL);
    /* Headless tests exercise CPU state without creating renderer resources. */
    Tether_Shape_API shape_api = *g_backend.shape;
    Tether_Text_API text_api = *g_backend.text;
    shape_api.is_alive = test_resource_alive;
    text_api.is_alive = test_resource_alive;
    shape_api.create = test_resource_create;
    text_api.create = test_resource_create;
    g_backend.shape = &shape_api;
    g_backend.text = &text_api;
    bool passed = false;
    if (!strcmp(argv[1], "layout")) {
        passed = layout_case();
    } else if (!strcmp(argv[1], "visibility")) {
        passed = visibility_case();
    } else if (!strcmp(argv[1], "input")) {
        passed = input_case();
    } else if (!strcmp(argv[1], "lifetime")) {
        passed = lifetime_case();
    } else if (!strcmp(argv[1], "text")) {
        passed = text_case();
    } else if (!strcmp(argv[1], "widgets")) {
        passed = widgets_case();
    } else if (!strcmp(argv[1], "menu_interaction")) {
        passed = menu_interaction_case();
    } else if (!strcmp(argv[1], "disabled")) {
        passed = disabled_case();
    } else if (!strcmp(argv[1], "event_contract")) {
        passed = event_contract_case();
    }
    tether_input_term();
    tether_events_term();
    tether_registry_term();
    tether_ecs_term();
    printf("%s: %s (%u checks)\n", argv[1], passed ? "PASSED" : "FAILED", checks);
    return passed ? 0 : 1;
}
