#include "tether/tether.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_input.h"
#include "tether/core/tether_registry.h"
#include "tether/engine/tether_engine.h"
#include "tether/engine/tether_layout.h"
#include "tether/engine/parser/tether_yaml_ast.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* These cases exercise CPU-side contracts without opening a graphics device.
   They do not stand in for font rasterization or pixel-comparison tests. */
static unsigned checks;

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
    Tether_GUID entity = tether_widget_create_panel(parent);
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

static void count_click(Tether_GUID entity, Tether_EventType type, void *data) {
    (void)entity;
    if (type == TETHER_EVENT_CLICK) {
        (*(unsigned *)data)++;
    }
}

static bool input_case(void) {
    Tether_GUID root = panel(tether_ecs_get_main_root(), 200, 100);
    Tether_GUID lower = panel(root, 100, 100);
    Tether_GUID upper = panel(root, 100, 100);
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
    CHECK(!tether_ecs_set_text_string(child, "stale"));
    return true;
}

static bool text_case(void) {
    Tether_GUID text = tether_widget_create_text(tether_ecs_get_main_root());
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

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: tether_behavior_test layout|visibility|input|lifetime|text\n");
        return 2;
    }
    tether_init(NULL);
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
    }
    tether_input_term();
    tether_events_term();
    tether_registry_term();
    tether_ecs_term();
    printf("%s: %s (%u checks)\n", argv[1], passed ? "PASSED" : "FAILED", checks);
    return passed ? 0 : 1;
}
