#include "tether/tether.h"
#include "tether/backends/tether_backend.h"
#include "tether/core/tether_input.h"
#include "tether/core/tether_registry.h"
#include "tether/engine/parser/tether_yaml.h"
#include "tether/ui/tether_widgets.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/widgets/tether_button.h"
#include "tether/ui/widgets/tether_dropdown.h"
#include "tether/ui/widgets/tether_menu.h"

#include <stdio.h>
#include <string.h>

static unsigned checks;
static const char* fixture;
static Tether_GUID parent;

#define CHECK(condition) do { \
    ++checks; \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        return false; \
    } \
} while (0)

static bool write_fixture(const char* yaml) {
    FILE* file = fopen(fixture, "wb");
    if (!file) return false;
    bool written = fputs(yaml, file) >= 0;
    return fclose(file) == 0 && written;
}

static Tether_GUID load(const char* yaml) {
    return write_fixture(yaml) ? tether_yaml_load(fixture, parent) : TETHER_INVALID_GUID;
}

static uint32_t entity_count(void) {
    return tether_ecs_get_dense_array(TETHER_COMPONENT_SLOT_TRANSFORM)->count;
}

static bool rejected(const char* yaml) {
    uint32_t before = entity_count();
    CHECK(write_fixture(yaml));
    CHECK(tether_yaml_load(fixture, parent) == TETHER_INVALID_GUID);
    CHECK(entity_count() == before);
    return true;
}

static bool dropdown_case(void) {
    Tether_GUID first = load("Dropdown: {selected: 2, open: true, options: [One, Two, Three]}\n");
    CHECK(tether_ecs_is_valid(first));
    CHECK(tether_ui_get_dropdown_selected(first) == 2);
    CHECK(tether_ui_is_dropdown_open(first));
    tether_ui_set_dropdown_open(first, false);
    Tether_GUID second = load("Dropdown: {options: [One, Two, Three], open: false, selected: 2}\n");
    CHECK(tether_ecs_is_valid(second));
    CHECK(!tether_ui_is_dropdown_open(second));
    CHECK(tether_ui_get_dropdown_selected(first) == tether_ui_get_dropdown_selected(second));
    Tether_GUID header = tether_widget_get_child_at(second, 0);
    CHECK(!strcmp(tether_ui_get_button_label(header), "Three"));

    Tether_GUID native = tether_ui_dropdown(parent);
    const char* labels[] = {"One", "Two", "Three"};
    tether_ui_set_dropdown_options(native, labels, 3);
    tether_ui_set_dropdown_selected(native, 2);
    CHECK(tether_ui_get_dropdown_selected(native) == tether_ui_get_dropdown_selected(second));

    FILE* file = fopen(fixture, "wb");
    CHECK(file != NULL);
    fputs("Dropdown:\n  selected: 39\n  options:\n", file);
    for (int i = 0; i < 40; ++i) fprintf(file, "    - Item %d\n", i);
    CHECK(fclose(file) == 0);
    Tether_GUID large = tether_yaml_load(fixture, parent);
    CHECK(tether_ecs_is_valid(large));
    Tether_GUID menu = tether_widget_get_child_at(large, 1);
    CHECK(tether_ui_menu_get_item_count(menu) == 40);
    CHECK(tether_ui_get_dropdown_selected(large) == 39);
    CHECK(!strcmp(tether_ui_menu_get_item_text(menu, 39), "Item 39"));
    return true;
}

static bool template_case(void) {
    Tether_GUID first = load(
        "- Widget: {name: Caption, tree: {Text: {string: $caption}}}\n"
        "- Caption: {caption: First}\n");
    CHECK(tether_ecs_is_valid(first));
    CHECK(!strcmp(tether_ui_get_text(first), "First"));
    Tether_GUID second = load("Caption: {caption: Second}\n");
    CHECK(tether_ecs_is_valid(second));
    CHECK(!strcmp(tether_ui_get_text(second), "Second"));
    Tether_GUID replaced = load(
        "- Widget: {name: Caption, tree: {Text: {string: Replacement}}}\n"
        "- Caption: {}\n");
    CHECK(tether_ecs_is_valid(replaced));
    CHECK(!strcmp(tether_ui_get_text(replaced), "Replacement"));
    CHECK(rejected("- Widget: {name: Loop, tree: {Loop: {}}}\n- Loop: {}\n"));
    return true;
}

static bool invalid_case(void) {
    CHECK(rejected("Dropdown: {options: [One], selected: 1}\n"));
    CHECK(rejected("Dropdown: {options: [One], selected: 0.5}\n"));
    CHECK(rejected("Dropdown: {options: [One], selected: 999999999999999999999}\n"));
    CHECK(rejected("Dropdown: {options: [One], open: perhaps}\n"));
    CHECK(rejected("Dropdown: {options: [One, {bad: item}]}\n"));
    CHECK(rejected("Dropdown: {options: wrong}\n"));
    CHECK(rejected("- Button: {id: discarded}\n- Dropdown: {selected: 5}\n"));
    CHECK(tether_registry_find_entity("discarded") == TETHER_INVALID_GUID);
    CHECK(rejected("Container: {children: [{Button: {}}, {UnknownWidget: {}}]}\n"));
    CHECK(tether_ecs_is_valid(load("Dropdown: {options: [Recovered], selected: 0}\n")));
    return true;
}

static bool resource_alive(uint32_t handle) {
    (void)handle;
    return false;
}

static uint32_t resource_create(void) {
    return 0;
}

static void resolution(uint32_t* width, uint32_t* height) {
    *width = 800;
    *height = 600;
}

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    fixture = argv[1];
    tether_init(NULL);
    /* CPU integration only: do not allocate renderer resources without a graphics device. */
    Tether_Shape_API shape = *g_backend.shape;
    Tether_Text_API text = *g_backend.text;
    Tether_System_API system = *g_backend.system;
    shape.is_alive = resource_alive;
    shape.create = resource_create;
    text.is_alive = resource_alive;
    text.create = resource_create;
    system.get_resolution = resolution;
    g_backend.shape = &shape;
    g_backend.text = &text;
    g_backend.system = &system;
    parent = tether_widget_create_container(tether_ecs_get_main_root());
    bool passed = dropdown_case() && template_case() && invalid_case();
    /* Optional existing example documents exercise compatibility with real YAML. */
    for (int i = 2; passed && i < argc; ++i) {
        Tether_GUID example = tether_yaml_load(argv[i], parent);
        ++checks;
        if (!tether_ecs_is_valid(example)) {
            fprintf(stderr, "Example failed to load: %s\n", argv[i]);
            passed = false;
        }
    }
    tether_input_term();
    tether_widget_destroy(parent);
    tether_yaml_term();
    tether_events_term();
    tether_registry_term();
    tether_ecs_term();
    remove(fixture);
    printf("yaml_construction: %s (%u checks)\n", passed ? "PASSED" : "FAILED", checks);
    return passed ? 0 : 1;
}
