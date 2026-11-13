#include "app.h"
#include "sandbox_events.h"
#include "tether/ui/tether_ui.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_registry.h"
#include "tether/ui/tether_widgets.h"
#include "tether/engine/parser/tether_yaml.h"
#include "tether/backends/tether_backend.h"
#include "tool.h"
#include <stdio.h>
#include <string.h>

static Tether_GUID preview_content = TETHER_INVALID_GUID;
static Tether_GUID dialog_overlay = TETHER_INVALID_GUID;
static Tether_GUID toast_overlay = TETHER_INVALID_GUID;
static double toast_deadline;
static unsigned example_color_index;
static unsigned example_align_index;
static unsigned example_font_index;
static unsigned example_text_color_index;
static unsigned example_border_index;
static unsigned example_radius_index;
static unsigned example_padding_index;
static unsigned example_gap_index;
static unsigned example_progress_index;
static unsigned example_opacity_index;
static unsigned example_rotation_index;
static unsigned example_scale_index;
static bool example_vertical;
static bool example_alternate_text;
static char chosen_yaml[4096];
static char active_yaml[4096];
static uint64_t active_yaml_mtime;
static double last_sync_poll_ms;
static bool live_sync_active = true;

static void size_overlay(Tether_GUID overlay) {
    if (!tether_ecs_is_valid(overlay) || !g_backend.system || !g_backend.system->get_resolution) return;
    uint32_t width = 0, height = 0;
    g_backend.system->get_resolution(&width, &height);
    Tether_SlotTransform *slot = tether_ecs_get_component(overlay, TETHER_COMPONENT_SLOT_TRANSFORM);
    if (slot && (slot->width != (float)width || slot->height != (float)height)) {
        slot->x = 0;
        slot->y = 0;
        slot->width = (float)width;
        slot->height = (float)height;
        tether_ecs_add_component(overlay, TETHER_COMPONENT_DIRTY_LAYOUT);
    }
}

void sandbox_showcase_close_overlays(void) {
    toast_deadline = 0;
    if (tether_ecs_is_valid(dialog_overlay)) tether_widget_hide(dialog_overlay);
    if (tether_ecs_is_valid(toast_overlay)) tether_widget_hide(toast_overlay);
}

void sandbox_showcase_cleanup(void) {
    sandbox_showcase_close_overlays();
    if (tether_ecs_is_valid(dialog_overlay)) tether_widget_destroy(dialog_overlay);
    if (tether_ecs_is_valid(toast_overlay)) tether_widget_destroy(toast_overlay);
    if (tether_ecs_is_valid(preview_content)) tether_widget_destroy(preview_content);
    dialog_overlay = TETHER_INVALID_GUID;
    toast_overlay = TETHER_INVALID_GUID;
    preview_content = TETHER_INVALID_GUID;
    active_yaml[0] = '\0';
    active_yaml_mtime = 0;
}

static void select_showcase_view(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)event;
    (void)data;
    tether_ui_switcher_show_index(tether_registry_find_entity("showcase_switcher"),
                                  tether_ui_get_segment_selected(entity));
}

static void show_dialog(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    size_overlay(dialog_overlay);
    tether_widget_show(dialog_overlay);
}

static void close_dialog(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    tether_widget_hide(dialog_overlay);
}

static void close_toast(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    toast_deadline = 0;
    tether_widget_hide(toast_overlay);
}

static void show_toast(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    size_overlay(toast_overlay);
    tether_widget_show(toast_overlay);
    toast_deadline = tool_now_ms() + 4000;
}

static bool load_preview(const char *path);

void sandbox_showcase_update(void) {
    if (toast_deadline && tool_now_ms() >= toast_deadline) close_toast(0, NULL, NULL);
    Tether_Visibility *dialog = tether_ecs_get_component(dialog_overlay, TETHER_COMPONENT_VISIBILITY);
    Tether_Visibility *toast = tether_ecs_get_component(toast_overlay, TETHER_COMPONENT_VISIBILITY);
    if (dialog && dialog->local_state == TETHER_VISIBLE) size_overlay(dialog_overlay);
    if (toast && toast->local_state == TETHER_VISIBLE) size_overlay(toast_overlay);

    if (live_sync_active && active_yaml[0]) {
        double now = tool_now_ms();
        if (now - last_sync_poll_ms >= 250.0) {
            last_sync_poll_ms = now;
            uint64_t mtime = tool_file_mtime(active_yaml);
            if (mtime != 0 && mtime != active_yaml_mtime) {
                active_yaml_mtime = mtime;
                load_preview(active_yaml);
            }
        }
    }
}

static void clear_preview(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    if (tether_ecs_is_valid(preview_content)) tether_widget_destroy(preview_content);
    preview_content = TETHER_INVALID_GUID;
    active_yaml[0] = '\0';
    active_yaml_mtime = 0;
    tether_widget_set_enabled(tether_registry_find_entity("showcase_sync_yaml"), false);
    tether_ui_set_text(tether_registry_find_entity("showcase_preview_status"), "Preview cleared.");
}

static bool load_preview(const char *path) {
    if (!path || !path[0]) return false;
    if (active_yaml != path) {
        snprintf(active_yaml, sizeof active_yaml, "%s", path);
    }
    active_yaml_mtime = tool_file_mtime(path);
    tether_widget_set_enabled(tether_registry_find_entity("showcase_sync_yaml"), true);
    if (tether_ecs_is_valid(preview_content)) {
        tether_widget_destroy(preview_content);
        preview_content = TETHER_INVALID_GUID;
    }
    Tether_GUID host = tether_registry_find_entity("showcase_preview");
    preview_content = tether_widget_create_container(host);
    tether_ui_set_container(preview_content, TETHER_FLOW_COLUMN);
    tether_ui_set_flex(preview_content, 1.0f);
    if (tether_yaml_load(path, preview_content) == TETHER_INVALID_GUID) {
        tether_widget_destroy(preview_content);
        preview_content = TETHER_INVALID_GUID;
        tether_ui_set_text(tether_registry_find_entity("showcase_preview_status"),
                           "Could not load YAML. Check the file and console for details.");
        return false;
    }
    /* Each top-level entity shares the preview; its descendants keep their own layout. */
    int count = tether_widget_get_child_count(preview_content);
    for (int i = 0; i < count; ++i) {
        Tether_GUID root = tether_widget_get_child_at(preview_content, i);
        tether_ui_set_size(root, 0.0f, 0.0f);
        tether_ui_set_flex(root, 1.0f);
        Tether_FlexSlot *slot = tether_ecs_get_component(root, TETHER_COMPONENT_FLEX_SLOT);
        if (slot) slot->align_self_x = TETHER_ALIGN_FILL;
    }
    tether_widget_show(preview_content);
    tether_ui_set_text(tether_registry_find_entity("showcase_preview_status"),
                       "YAML loaded. Edit and save to live sync, or click Sync.");
    return true;
}

static void sync_yaml(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    if (active_yaml[0]) {
        if (load_preview(active_yaml)) {
            tether_ui_set_text(tether_registry_find_entity("showcase_preview_status"), "Preview synced.");
        }
    }
}

static void toggle_live_sync(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)event; (void)data;
    live_sync_active = tether_ui_get_toggle(entity);
    if (live_sync_active) {
        last_sync_poll_ms = 0;
        if (active_yaml[0]) active_yaml_mtime = tool_file_mtime(active_yaml);
    }
}

static void choose_yaml(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    char path[4096];
#ifdef _WIN32
    if (tool_pick_yaml(path, sizeof path) && load_preview(path)) {
        snprintf(chosen_yaml, sizeof chosen_yaml, "%s", path);
        tether_widget_set_enabled(tether_registry_find_entity("showcase_selected_yaml"), true);
        tether_ui_set_text(tether_registry_find_entity("showcase_preview_status"), "File loaded. Use Chosen file to return to it.");
    }
#else
    (void)path;
    tether_ui_set_text(tether_registry_find_entity("showcase_preview_status"),
                       "The file picker is available on Windows; use the included sample here.");
#endif
}

static void load_preset(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity;
    (void)event;
    load_preview((const char *)data);
}

static void load_chosen_yaml(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    if (chosen_yaml[0]) load_preview(chosen_yaml);
}

static void toggle_example_visibility(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    Tether_GUID target = tether_registry_find_entity("example_target");
    Tether_Visibility *visibility = tether_ecs_get_component(target, TETHER_COMPONENT_VISIBILITY);
    if (visibility) tether_widget_set_visibility(target, visibility->local_state == TETHER_VISIBLE
                                                          ? TETHER_HIDDEN : TETHER_VISIBLE);
}

static void change_example_color(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    const Tether_Color colors[] = {{48, 68, 85, 255}, {63, 85, 65, 255}, {92, 65, 67, 255}};
    example_color_index = (example_color_index + 1) % 3;
    tether_ui_set_color(tether_registry_find_entity("example_target"), colors[example_color_index]);
}

static void change_example_alignment(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    const Tether_Align alignments[] = {TETHER_ALIGN_START, TETHER_ALIGN_CENTER, TETHER_ALIGN_END};
    example_align_index = (example_align_index + 1) % 3;
    tether_ui_set_text_align(tether_registry_find_entity("example_text"),
                             alignments[example_align_index], TETHER_ALIGN_CENTER);
}

static void toggle_example_enabled(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    Tether_GUID target = tether_registry_find_entity("example_target_button");
    tether_widget_set_enabled(target, !tether_widget_is_enabled(target));
}

static void change_example_font_size(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    const float sizes[] = {17.0f, 22.0f, 28.0f};
    example_font_index = (example_font_index + 1) % 3;
    tether_ui_set_text_size(tether_registry_find_entity("example_text"), sizes[example_font_index]);
}

static void change_example_text_color(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    const Tether_Color colors[] = {{230, 230, 230, 255}, {145, 215, 180, 255}, {245, 205, 140, 255}};
    example_text_color_index = (example_text_color_index + 1) % 3;
    tether_ui_set_text_color(tether_registry_find_entity("example_text"), colors[example_text_color_index]);
}

static void update_example_border(void) {
    const float widths[] = {0.0f, 2.0f, 4.0f};
    const float radii[] = {7.0f, 0.0f, 20.0f};
    tether_ui_set_border(tether_registry_find_entity("example_target"), widths[example_border_index],
                         (Tether_Color){160, 190, 215, 255}, radii[example_radius_index]);
}

static void change_example_border(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    example_border_index = (example_border_index + 1) % 3;
    update_example_border();
}

static void change_example_radius(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    example_radius_index = (example_radius_index + 1) % 3;
    update_example_border();
}

static void change_example_padding(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    const float padding[] = {12.0f, 20.0f, 4.0f};
    example_padding_index = (example_padding_index + 1) % 3;
    float value = padding[example_padding_index];
    tether_ui_set_padding(tether_registry_find_entity("example_target"), value, value, value, value);
}

static void change_example_gap(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    const float gaps[] = {10.0f, 24.0f, 2.0f};
    example_gap_index = (example_gap_index + 1) % 3;
    tether_ui_set_gap(tether_registry_find_entity("example_card_row"),
                      gaps[example_gap_index], gaps[example_gap_index]);
}

static void change_example_flow(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    example_vertical = !example_vertical;
    tether_ui_set_container(tether_registry_find_entity("example_card_row"),
                            example_vertical ? TETHER_FLOW_COLUMN : TETHER_FLOW_ROW);
}

static void change_example_text(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    example_alternate_text = !example_alternate_text;
    tether_ui_set_text(tether_registry_find_entity("example_card_text"),
                       example_alternate_text ? "Review in progress" : "Preparing a preview");
}

static void change_example_progress(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    const float values[] = {0.35f, 0.65f, 1.0f};
    example_progress_index = (example_progress_index + 1) % 3;
    tether_ui_set_progress(tether_registry_find_entity("example_progress"), values[example_progress_index]);
}

static void change_example_opacity(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    const float values[] = {1.0f, 0.5f, 0.2f};
    example_opacity_index = (example_opacity_index + 1) % 3;
    Tether_GUID badge = tether_registry_find_entity("example_badge");
    Tether_RenderTransform *transform = tether_ecs_get_component(badge, TETHER_COMPONENT_RENDER_TRANSFORM);
    if (transform) {
        transform->opacity = values[example_opacity_index];
        tether_ecs_add_component(badge, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

static void rotate_example_badge(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    const float angles[] = {0.0f, 45.0f, 90.0f};
    example_rotation_index = (example_rotation_index + 1) % 3;
    Tether_GUID badge = tether_registry_find_entity("example_badge");
    Tether_RenderTransform *transform = tether_ecs_get_component(badge, TETHER_COMPONENT_RENDER_TRANSFORM);
    if (transform) {
        transform->rotation_deg = angles[example_rotation_index];
        tether_ecs_add_component(badge, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

static void scale_example_badge(Tether_GUID entity, const Tether_Event *event, void *data) {
    (void)entity; (void)event; (void)data;
    const float scales[] = {1.0f, 0.7f, 1.2f};
    example_scale_index = (example_scale_index + 1) % 3;
    Tether_GUID badge = tether_registry_find_entity("example_badge");
    Tether_RenderTransform *transform = tether_ecs_get_component(badge, TETHER_COMPONENT_RENDER_TRANSFORM);
    if (transform) {
        transform->scale_x = scales[example_scale_index];
        transform->scale_y = scales[example_scale_index];
        tether_ecs_add_component(badge, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

bool sandbox_showcase_check(void) {
    Tether_GUID tabs = tether_registry_find_entity("showcase_tabs");
    Tether_GUID switcher = tether_registry_find_entity("showcase_switcher");
    Tether_GUID dialog = tether_registry_find_entity("showcase_dialog");
    Tether_GUID toast = tether_registry_find_entity("showcase_toast");
    if (!tabs || !switcher || !dialog || !toast) return false;

    tether_ui_set_segment_selected(tabs, 1);
    if (tether_ui_switcher_get_active(switcher) != 1) return false;
    tether_dispatch_event(tether_registry_find_entity("showcase_open_dialog"), TETHER_EVENT_CLICK);
    Tether_Visibility *visibility = tether_ecs_get_component(dialog, TETHER_COMPONENT_VISIBILITY);
    if (!visibility || visibility->local_state != TETHER_VISIBLE) return false;
    tether_dispatch_event(tether_registry_find_entity("showcase_dialog_close"), TETHER_EVENT_CLICK);
    if (visibility->local_state != TETHER_COLLAPSED) return false;
    tether_dispatch_event(tether_registry_find_entity("showcase_show_toast"), TETHER_EVENT_CLICK);
    visibility = tether_ecs_get_component(toast, TETHER_COMPONENT_VISIBILITY);
    if (!visibility || visibility->local_state != TETHER_VISIBLE || !toast_deadline) return false;
    tether_dispatch_event(tether_registry_find_entity("showcase_toast_close"), TETHER_EVENT_CLICK);
    if (visibility->local_state != TETHER_COLLAPSED || toast_deadline) return false;

    Tether_GUID target = tether_registry_find_entity("example_target");
    Tether_GUID button = tether_registry_find_entity("example_target_button");
    Tether_GUID label = tether_registry_find_entity("example_text");
    tether_dispatch_event(tether_registry_find_entity("example_visibility"), TETHER_EVENT_CLICK);
    if (tether_widget_is_visible(target)) return false;
    tether_dispatch_event(tether_registry_find_entity("example_visibility"), TETHER_EVENT_CLICK);
    if (!tether_widget_is_visible(target)) return false;
    tether_dispatch_event(tether_registry_find_entity("example_enable"), TETHER_EVENT_CLICK);
    if (tether_widget_is_enabled(button)) return false;
    tether_dispatch_event(tether_registry_find_entity("example_enable"), TETHER_EVENT_CLICK);
    if (!tether_widget_is_enabled(button)) return false;
    tether_dispatch_event(tether_registry_find_entity("example_color"), TETHER_EVENT_CLICK);
    if (tether_ui_get_color(target).g != 85) return false;
    tether_dispatch_event(tether_registry_find_entity("example_align"), TETHER_EVENT_CLICK);
    Tether_Text *text = tether_ecs_get_component(label, TETHER_COMPONENT_TEXT);
    if (!text || text->align_x != TETHER_ALIGN_CENTER) return false;
    tether_dispatch_event(tether_registry_find_entity("example_font_size"), TETHER_EVENT_CLICK);
    text = tether_ecs_get_component(label, TETHER_COMPONENT_TEXT);
    if (!text || text->font_size != 22.0f) return false;
    tether_dispatch_event(tether_registry_find_entity("example_text_color"), TETHER_EVENT_CLICK);
    text = tether_ecs_get_component(label, TETHER_COMPONENT_TEXT);
    if (!text || text->color.g != 215) return false;
    tether_dispatch_event(tether_registry_find_entity("example_border"), TETHER_EVENT_CLICK);
    tether_dispatch_event(tether_registry_find_entity("example_radius"), TETHER_EVENT_CLICK);
    Tether_Shape *shape = tether_ecs_get_component(target, TETHER_COMPONENT_SHAPE);
    if (!shape || shape->border_width != 2.0f || shape->border_radius.top != 0.0f) return false;
    tether_dispatch_event(tether_registry_find_entity("example_padding"), TETHER_EVENT_CLICK);
    Tether_Layout *layout = tether_ecs_get_component(target, TETHER_COMPONENT_LAYOUT);
    if (!layout || layout->padding.top != 20.0f) return false;
    tether_dispatch_event(tether_registry_find_entity("example_gap"), TETHER_EVENT_CLICK);
    tether_dispatch_event(tether_registry_find_entity("example_flow"), TETHER_EVENT_CLICK);
    layout = tether_ecs_get_component(tether_registry_find_entity("example_card_row"), TETHER_COMPONENT_LAYOUT);
    if (!layout || layout->gap.x != 24.0f || layout->flow != TETHER_FLOW_COLUMN) return false;
    tether_dispatch_event(tether_registry_find_entity("example_copy"), TETHER_EVENT_CLICK);
    const char *card_text = tether_ui_get_text(tether_registry_find_entity("example_card_text"));
    if (!card_text || strcmp(card_text, "Review in progress")) return false;
    tether_dispatch_event(tether_registry_find_entity("example_progress_step"), TETHER_EVENT_CLICK);
    if (tether_ui_get_progress(tether_registry_find_entity("example_progress")) < 0.64f) return false;
    tether_dispatch_event(tether_registry_find_entity("example_opacity"), TETHER_EVENT_CLICK);
    Tether_RenderTransform *badge = tether_ecs_get_component(tether_registry_find_entity("example_badge"),
                                                              TETHER_COMPONENT_RENDER_TRANSFORM);
    if (!badge || badge->opacity != 0.5f) return false;
    tether_dispatch_event(tether_registry_find_entity("example_rotate"), TETHER_EVENT_CLICK);
    tether_dispatch_event(tether_registry_find_entity("example_scale"), TETHER_EVENT_CLICK);
    badge = tether_ecs_get_component(tether_registry_find_entity("example_badge"), TETHER_COMPONENT_RENDER_TRANSFORM);
    if (!badge || badge->rotation_deg != 45.0f || badge->scale_x != 0.7f || badge->scale_y != 0.7f) return false;

    tether_ui_set_segment_selected(tabs, 2);
    if (tether_ui_switcher_get_active(switcher) != 2 || !load_preview("UI/playground.yaml")) return false;
    if (!tether_ecs_is_valid(preview_content) || tether_widget_get_child_count(preview_content) == 0) return false;
    Tether_GUID sync_button = tether_registry_find_entity("showcase_sync_yaml");
    Tether_GUID live_sync_toggle = tether_registry_find_entity("showcase_live_sync");
    if (!sync_button || !live_sync_toggle || !tether_widget_is_enabled(sync_button)) return false;
    tether_dispatch_event(sync_button, TETHER_EVENT_CLICK);
    const char *sync_status = tether_ui_get_text(tether_registry_find_entity("showcase_preview_status"));
    if (!sync_status || strcmp(sync_status, "Preview synced.") != 0) return false;
    tether_ui_set_toggle(live_sync_toggle, false);
    if (live_sync_active) return false;
    tether_ui_set_toggle(live_sync_toggle, true);
    if (!live_sync_active) return false;
    if (!load_preview("UI/playground/layout.yaml")) return false;
    char previous_yaml[sizeof chosen_yaml];
    memcpy(previous_yaml, chosen_yaml, sizeof previous_yaml);
    snprintf(chosen_yaml, sizeof chosen_yaml, "%s", "UI/playground.yaml");
    Tether_GUID chosen_button = tether_registry_find_entity("showcase_selected_yaml");
    tether_widget_set_enabled(chosen_button, true);
    tether_dispatch_event(chosen_button, TETHER_EVENT_CLICK);
    bool chosen_loaded = tether_ecs_is_valid(preview_content) && tether_widget_get_child_count(preview_content) > 0;
    memcpy(chosen_yaml, previous_yaml, sizeof chosen_yaml);
    tether_widget_set_enabled(chosen_button, chosen_yaml[0] != '\0');
    if (!chosen_loaded) return false;
    if (load_preview("UI/nonexistent-showcase-smoke.yaml") || tether_ecs_is_valid(preview_content)) return false;
    const char *status = tether_ui_get_text(tether_registry_find_entity("showcase_preview_status"));
    if (!status || !strstr(status, "Could not load")) return false;
    clear_preview(0, NULL, NULL);
    if (tether_widget_is_enabled(sync_button)) return false;
    tether_ui_set_segment_selected(tabs, 0);
    return true;
}

void sandbox_bind_events(void) {
    preview_content = TETHER_INVALID_GUID;
    toast_deadline = 0;
    active_yaml[0] = '\0';
    active_yaml_mtime = 0;
    last_sync_poll_ms = 0;
    live_sync_active = true;
    example_color_index = 0;
    example_align_index = 0;
    example_font_index = 0;
    example_text_color_index = 0;
    example_border_index = 0;
    example_radius_index = 0;
    example_padding_index = 0;
    example_gap_index = 0;
    example_progress_index = 0;
    example_opacity_index = 0;
    example_rotation_index = 0;
    example_scale_index = 0;
    example_vertical = false;
    example_alternate_text = false;
    dialog_overlay = tether_yaml_load("UI/showcase_dialog.yaml", tether_ecs_get_overlay_root());
    toast_overlay = tether_yaml_load("UI/showcase_toast.yaml", tether_ecs_get_overlay_root());
    sandbox_showcase_close_overlays();
    tether_bind_event(tether_registry_find_entity("showcase_tabs"), TETHER_EVENT_CHANGE, select_showcase_view, NULL);
    tether_bind_event(tether_registry_find_entity("showcase_open_dialog"), TETHER_EVENT_CLICK, show_dialog, NULL);
    tether_bind_event(tether_registry_find_entity("showcase_dialog_close"), TETHER_EVENT_CLICK, close_dialog, NULL);
    tether_bind_event(tether_registry_find_entity("showcase_show_toast"), TETHER_EVENT_CLICK, show_toast, NULL);
    tether_bind_event(tether_registry_find_entity("showcase_toast_close"), TETHER_EVENT_CLICK, close_toast, NULL);
    tether_bind_event(tether_registry_find_entity("showcase_load_yaml"), TETHER_EVENT_CLICK, choose_yaml, NULL);
    tether_bind_event(tether_registry_find_entity("showcase_preset_controls"), TETHER_EVENT_CLICK,
                      load_preset, "UI/playground.yaml");
    tether_bind_event(tether_registry_find_entity("showcase_preset_layout"), TETHER_EVENT_CLICK,
                      load_preset, "UI/playground/layout.yaml");
    tether_bind_event(tether_registry_find_entity("showcase_selected_yaml"), TETHER_EVENT_CLICK, load_chosen_yaml, NULL);
    tether_widget_set_enabled(tether_registry_find_entity("showcase_selected_yaml"), chosen_yaml[0] != '\0');
    tether_bind_event(tether_registry_find_entity("showcase_clear_preview"), TETHER_EVENT_CLICK, clear_preview, NULL);
    tether_bind_event(tether_registry_find_entity("showcase_sync_yaml"), TETHER_EVENT_CLICK, sync_yaml, NULL);
    tether_widget_set_enabled(tether_registry_find_entity("showcase_sync_yaml"), false);
    Tether_GUID live_sync_toggle = tether_registry_find_entity("showcase_live_sync");
    if (live_sync_toggle != TETHER_INVALID_GUID) {
        tether_ui_set_toggle(live_sync_toggle, true);
        tether_bind_event(live_sync_toggle, TETHER_EVENT_CHANGE, toggle_live_sync, NULL);
    }
    tether_bind_event(tether_registry_find_entity("example_visibility"), TETHER_EVENT_CLICK, toggle_example_visibility, NULL);
    tether_bind_event(tether_registry_find_entity("example_color"), TETHER_EVENT_CLICK, change_example_color, NULL);
    tether_bind_event(tether_registry_find_entity("example_align"), TETHER_EVENT_CLICK, change_example_alignment, NULL);
    tether_bind_event(tether_registry_find_entity("example_enable"), TETHER_EVENT_CLICK, toggle_example_enabled, NULL);
    tether_bind_event(tether_registry_find_entity("example_font_size"), TETHER_EVENT_CLICK, change_example_font_size, NULL);
    tether_bind_event(tether_registry_find_entity("example_text_color"), TETHER_EVENT_CLICK, change_example_text_color, NULL);
    tether_bind_event(tether_registry_find_entity("example_border"), TETHER_EVENT_CLICK, change_example_border, NULL);
    tether_bind_event(tether_registry_find_entity("example_radius"), TETHER_EVENT_CLICK, change_example_radius, NULL);
    tether_bind_event(tether_registry_find_entity("example_padding"), TETHER_EVENT_CLICK, change_example_padding, NULL);
    tether_bind_event(tether_registry_find_entity("example_gap"), TETHER_EVENT_CLICK, change_example_gap, NULL);
    tether_bind_event(tether_registry_find_entity("example_flow"), TETHER_EVENT_CLICK, change_example_flow, NULL);
    tether_bind_event(tether_registry_find_entity("example_copy"), TETHER_EVENT_CLICK, change_example_text, NULL);
    tether_bind_event(tether_registry_find_entity("example_progress_step"), TETHER_EVENT_CLICK, change_example_progress, NULL);
    tether_bind_event(tether_registry_find_entity("example_opacity"), TETHER_EVENT_CLICK, change_example_opacity, NULL);
    tether_bind_event(tether_registry_find_entity("example_rotate"), TETHER_EVENT_CLICK, rotate_example_badge, NULL);
    tether_bind_event(tether_registry_find_entity("example_scale"), TETHER_EVENT_CLICK, scale_example_badge, NULL);
}
