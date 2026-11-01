#include "tether/tether.h"
#include "tether/backends/tether_backend.h"
#include "tether/core/tether_registry.h"
#include "tether/engine/parser/tether_yaml.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include "chitra.h"
#include "chitra_clip.h"
#include "chitra_table.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static struct {
    const char *font_path;
    const char *yaml_path;
    bool volatile_mode;
    bool finished;
    unsigned frame;
    unsigned checks;
    unsigned failures;
    Tether_GUID text;
    Tether_GUID root;
    Tether_GUID hidden_parent;
    Tether_GUID hidden_text;
    Tether_GUID outer_clip;
    Tether_GUID inner_clip;
    Tether_GUID clipped_shape;
    Tether_GUID yaml_root;
    float original_x;
    float original_y;
    float original_slot_x;
    float original_slot_y;
    float wrapped_x;
    float wrapped_y;
} test;

static void check(bool condition, const char *description) {
    test.checks++;
    if (!condition) {
        test.failures++;
        fprintf(stderr, "Frame %u: %s\n", test.frame, description);
    }
}

static bool write_scene(const char *contents) {
    FILE *file = fopen(test.yaml_path, "wb");
    if (!file) {
        return false;
    }
    bool ok = fputs(contents, file) >= 0;
    if (fclose(file)) {
        ok = false;
    }
    return ok;
}

static Tether_GUID panel(Tether_GUID parent, float width, float height, float x, float y) {
    Tether_GUID entity = tether_widget_create_panel(parent);
    tether_ui_set_size(entity, width, height);
    Tether_AnchorSlot *slot = tether_ecs_get_component(entity, TETHER_COMPONENT_ANCHOR_SLOT);
    slot->anchor_min = (Tether_Vec2){0, 0};
    slot->anchor_max = (Tether_Vec2){0, 0};
    slot->offset.left = x;
    slot->offset.top = y;
    slot->offset.right = -(x + width);
    slot->offset.bottom = -(y + height);
    return entity;
}

static void apply_mode(void) {
    if (!test.volatile_mode) {
        return;
    }
    Tether_DenseArray *entities = tether_ecs_get_dense_array(TETHER_COMPONENT_SLOT_TRANSFORM);
    for (uint32_t i = 0; i < entities->count; i++) {
        tether_ecs_add_component(entities->entity_map[i], TETHER_COMPONENT_VOLATILE);
    }
}

static bool first_vertex(Tether_GUID entity, float *x, float *y) {
    Tether_Text *text = tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT);
    if (!text) {
        return false;
    }
    uint8_t pipeline;
    uint32_t count = 0;
    Chitra_TextVertex *vertices = chitra_table_get_vertices(text->handle, &pipeline, &count);
    if (!vertices || !count || pipeline != CHITRA_PIPELINE_TEXT) {
        return false;
    }
    *x = vertices[0].pos[0];
    *y = vertices[0].pos[1];
    return true;
}

static void init(void) {
    tether_font_register("Roboto-Regular", test.font_path);
    Tether_GUID stage = panel(tether_ecs_get_main_root(), 800, 600, 0, 0);
    tether_widget_show(stage);
    Tether_GUID root = panel(stage, 400, 100, 0, 0);
    test.root = root;
    test.text = tether_ui_text(root, "Tether validation", 24, (Tether_Color){255, 255, 255, 255});
    tether_ui_set_text_align(test.text, TETHER_ALIGN_START, TETHER_ALIGN_START);
    tether_widget_show(root);

    test.hidden_parent = panel(stage, 300, 100, 0, 250);
    test.hidden_text = tether_ui_text(test.hidden_parent, "Hidden original", 24,
                                      (Tether_Color){255, 255, 255, 255});
    tether_widget_set_visibility(test.hidden_parent, TETHER_HIDDEN);
    tether_widget_show(test.hidden_parent);

    test.outer_clip = panel(stage, 200, 100, 500, 30);
    Tether_Overflow *overflow =
        tether_ecs_add_component(test.outer_clip, TETHER_COMPONENT_OVERFLOW);
    *overflow = (Tether_Overflow){TETHER_OVERFLOW_CLIP, TETHER_OVERFLOW_CLIP};
    test.inner_clip = panel(test.outer_clip, 200, 100, 50, 20);
    overflow = tether_ecs_add_component(test.inner_clip, TETHER_COMPONENT_OVERFLOW);
    *overflow = (Tether_Overflow){TETHER_OVERFLOW_CLIP, TETHER_OVERFLOW_CLIP};
    test.clipped_shape = panel(test.inner_clip, 200, 100, 0, 0);
    tether_widget_show(test.outer_clip);

    check(write_scene("- Panel:\n    id: ReloadRoot\n    children:\n      - Text:\n          id: "
                      "ReloadText\n          string: before\n"),
          "write initial YAML");
    test.yaml_root = tether_yaml_load(test.yaml_path, TETHER_INVALID_GUID);
    check(tether_ecs_is_valid(test.yaml_root), "instantiate YAML scene");
    apply_mode();
}

static void update(void) {
    if (test.finished) {
        return;
    }
    if (test.frame == 1) {
        tether_ui_set_text_align(test.text, TETHER_ALIGN_CENTER, TETHER_ALIGN_CENTER);
        tether_ui_set_text(test.hidden_text, "Changed while hidden");
    } else if (test.frame == 2) {
        tether_widget_set_visibility(test.hidden_parent, TETHER_VISIBLE);
        Tether_Text *text = tether_ecs_get_component(test.text, TETHER_COMPONENT_TEXT);
        text->wrap_width = 100;
        tether_ui_set_text(test.text, "Tether validation wraps across several lines");
    } else if (test.frame == 3) {
        Tether_AnchorSlot *anchor =
            tether_ecs_get_component(test.root, TETHER_COMPONENT_ANCHOR_SLOT);
        anchor->offset.left = 30;
        anchor->offset.top = 20;
        anchor->offset.right = -430;
        anchor->offset.bottom = -120;
        tether_ecs_add_component(test.root, TETHER_COMPONENT_DIRTY_LAYOUT);
        Tether_GUID old_root = test.yaml_root;
        Tether_GUID old_text = tether_registry_find_entity("ReloadText");
        tether_widget_destroy(old_root);
        check(!tether_ecs_is_valid(old_root) && !tether_ecs_is_valid(old_text),
              "reload destroys old entities");
        check(write_scene("- Panel:\n    id: ReloadRoot\n    children:\n      - Text:\n          "
                          "id: ReloadText\n          string: after\n"),
              "write modified YAML");
        test.yaml_root = tether_yaml_load(test.yaml_path, TETHER_INVALID_GUID);
        check(tether_ecs_is_valid(test.yaml_root), "reload modified YAML");
        Tether_GUID new_text = tether_registry_find_entity("ReloadText");
        const char *value = tether_ecs_get_text_string(new_text);
        check(value && !strcmp(value, "after"), "reload uses modified content");
        check(write_scene("- Panel: [unterminated"), "write invalid YAML");
        check(tether_yaml_load(test.yaml_path, TETHER_INVALID_GUID) == TETHER_INVALID_GUID,
              "invalid YAML is rejected");
        check(tether_ecs_is_valid(test.yaml_root), "failed load preserves existing scene");
        apply_mode();
    }
}

static void after_frame(void) {
    if (test.finished) {
        return;
    }
    float x = 0, y = 0;
    check(first_vertex(test.text, &x, &y), "real font produces text geometry");
    Tether_Text *text = tether_ecs_get_component(test.text, TETHER_COMPONENT_TEXT);
    Tether_SlotTransform *bounds =
        tether_ecs_get_component(test.text, TETHER_COMPONENT_SLOT_TRANSFORM);
    if (test.frame == 0) {
        test.original_x = x;
        test.original_y = y;
        test.original_slot_x = bounds->x;
        test.original_slot_y = bounds->y;
        check(text && text->intrinsic_size.x > 0 && text->intrinsic_size.y > 0,
              "font measurement is nonzero");
    } else if (test.frame == 1 && text && bounds) {
        /* The first glyph's bearing must survive a position-only update. */
        float expected_x = test.original_x - test.original_slot_x + bounds->x +
                           (bounds->width - text->intrinsic_size.x) / 2;
        float expected_y = test.original_y - test.original_slot_y + bounds->y +
                           (bounds->height - text->intrinsic_size.y) / 2;
        if (fabsf(x - expected_x) >= 0.1f || fabsf(y - expected_y) >= 0.1f) {
            fprintf(stderr,
                    "Text origin %.2f,%.2f; actual %.2f,%.2f; expected %.2f,%.2f; slot %.2fx%.2f; "
                    "intrinsic %.2fx%.2f\n",
                    test.original_x, test.original_y, x, y, expected_x, expected_y, bounds->width,
                    bounds->height, text->intrinsic_size.x, text->intrinsic_size.y);
        }
        check(fabsf(x - expected_x) < 0.1f && fabsf(y - expected_y) < 0.1f,
              "alignment updates existing text geometry");
    } else if (test.frame == 2) {
        test.wrapped_x = x;
        test.wrapped_y = y;
        check(first_vertex(test.hidden_text, &x, &y),
              "hidden text becomes visible with pending content");
        check(text && text->intrinsic_size.y > 24, "wrapping changes measured height");
    } else if (test.frame == 3) {
        check(fabsf(x - (test.wrapped_x + 30)) < 0.1f && fabsf(y - (test.wrapped_y + 20)) < 0.1f,
              "parent movement preserves glyph geometry and alignment");
    }

    Tether_Shape *shape = tether_ecs_get_component(test.clipped_shape, TETHER_COMPONENT_SHAPE);
    uint16_t clip_id = shape ? g_rhi.system->get_clip_id(shape->handle) : 0;
    const Chitra_ClipRect *clip = chitra_get_clip(clip_id);
    if (test.frame == 0 && clip) {
        fprintf(stderr, "Clip id %u: %d,%d %dx%d\n", clip_id, clip->x, clip->y, clip->w, clip->h);
    }
    check(clip && clip->x == 550 && clip->y == 50 && clip->w == 150 && clip->h == 80,
          "nested clips intersect before graphics submission");
    if (++test.frame == 5) {
        test.finished = true;
        chitra_quit();
    }
}

int main(int argc, char **argv) {
    if (argc != 4 || (strcmp(argv[1], "retained") && strcmp(argv[1], "volatile"))) {
        fprintf(stderr,
                "Usage: tether_ui_contract_test retained|volatile FONT_PATH TEMP_YAML_PATH\n");
        return 2;
    }
    test.volatile_mode = !strcmp(argv[1], "volatile");
    test.font_path = argv[2];
    test.yaml_path = argv[3];
    Tether_App_Config config = {0};
    config.width = 800;
    config.height = 600;
    config.target_fps = 60;
    config.title = "Tether UI contract validation";
    config.on_init = init;
    config.on_frame = update;
    config.on_post_frame = after_frame;
    tether_run(&config);
    remove(test.yaml_path);
    printf("UI contracts / %s: %s (%u checks)\n", argv[1],
           test.finished && !test.failures ? "PASSED" : "FAILED", test.checks);
    return test.finished && !test.failures ? 0 : 1;
}
