#include "results.h"
#include "tether/ui/tether_ui.h"
#include "tool.h"
#include "tether/core/tether_registry.h"
#include "tether/ui/tether_widgets.h"
#include "chitra_table.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Only a bounded report snapshot enters the UI. Runner samples stay on disk. */
static Tether_GUID report_label, title_label, import_label, status_label, scenario_label;
static const struct {
    const char *id;
    ToolReportField field;
    unsigned format;
} metrics[] = {
    {"count", REPORT_COUNT, 0},     {"steps", REPORT_STEPS, 0},
    {"checks", REPORT_CHECKS, 0},    {"failures", REPORT_FAILURES, 0},
    {"work_mean", REPORT_MEAN, 1},      {"work_p95", REPORT_P95, 1},
    {"frame_mean", REPORT_FRAME_AVG, 1}, {"frame_p95", REPORT_FRAME_P95, 1},
    {"elapsed", REPORT_ELAPSED, 1},   {"cpu", REPORT_CPU, 1},
    {"rss", REPORT_RSS, 2},       {"draws", REPORT_DRAWS, 0},
    {"uploads", REPORT_UPLOADS, 2},   {"gpu", REPORT_GPU, 1}
};
#define CARD_COUNT (sizeof metrics / sizeof *metrics)
static Tether_GUID cards[CARD_COUNT], values[CARD_COUNT];

static void import_result(Tether_GUID id, const Tether_Event *event_info, void *data) {
    (void)event_info;
    (void)id;
    (void)data;
    char path[4096];
    if (tool_pick_result(path, sizeof path)) {
        results_load(path);
    }
}
static void set_status(const char *label, Tether_Color color) {
    tether_ui_set_text(status_label, label);
    tether_ui_set_text_color(status_label, color);
}
bool results_bind(void) {
    for (unsigned i = 0; i < CARD_COUNT; ++i) {
        char id[40];
        snprintf(id, sizeof id, "res_card_%s", metrics[i].id);
        cards[i] = tether_registry_find_entity(id);
        snprintf(id, sizeof id, "res_value_%s", metrics[i].id);
        values[i] = tether_registry_find_entity(id);
        if (!cards[i] || !values[i]) return false;
    }
    title_label = tether_registry_find_entity("workbench_title");
    import_label = tether_registry_find_entity("res_import_label");
    report_label = tether_registry_find_entity("res_report_text");
    status_label = tether_registry_find_entity("res_status_badge");
    scenario_label = tether_registry_find_entity("res_scenario_label");
    Tether_GUID import_button = tether_registry_find_entity("btn_load_result");
    if (!title_label || !import_label || !report_label || !status_label || !scenario_label || !import_button) return false;
    tether_bind_event(import_button, TETHER_EVENT_CLICK,
                      import_result, NULL);
    return true;
}
void results_message(const char *message) {
    tether_ui_set_text(report_label, message ? message : "");
    set_status("NOTICE", (Tether_Color){245, 195, 115, 255});
    tether_ui_set_text(scenario_label, "No result loaded");
    for (unsigned i = 0; i < CARD_COUNT; i++) {
        tether_ui_set_text(values[i], "--");
    }
}
bool results_load_after_process(const char *path, int exit_code) {
    ToolReport report;
    if (!tool_read_report_data(path, &report)) {
        results_message("Cannot load this file: incomplete or unsupported report.");
        return false;
    }
    const char *status = report.value[REPORT_STATUS];
    if (exit_code > 0 && !strcmp(status, "passed")) {
        results_message("The process exited with an error; its success report was not accepted.");
        return false;
    }
    if (!strcmp(status, "passed")) {
        set_status(!strcmp(report.value[REPORT_KIND], "benchmark") ? "COMPLETED" : "PASSED",
                   (Tether_Color){112, 222, 166, 255});
    } else if (!strcmp(status, "cancelled")) {
        set_status("CANCELLED", (Tether_Color){245, 195, 115, 255});
    } else if (!strcmp(status, "rejected")) {
        set_status("REJECTED", (Tether_Color){245, 195, 115, 255});
    } else {
        set_status("FAILED", (Tether_Color){255, 130, 140, 255});
    }
    char text[640];
    const char *scenario = report.value[REPORT_SCENARIO];
    bool rendered = !strcmp(scenario, "render") || !strcmp(scenario, "ui");
    snprintf(text, sizeof text, "%.100s / %.100s", scenario[0] ? scenario : "Unknown scenario",
             rendered ? report.value[REPORT_MODE] : "CPU only");
    tether_ui_set_text(scenario_label, text);
    for (unsigned i = 0; i < CARD_COUNT; i++) {
        const char *raw = report.value[metrics[i].field];
        char *end;
        double value = strtod(raw, &end);
        if (!raw[0] || *end || !isfinite(value) || value < 0) {
            bool failed_query = (metrics[i].field == REPORT_CPU &&
                                 !strcmp(report.value[REPORT_CPU_STATE], "query_failed")) ||
                                (metrics[i].field == REPORT_RSS &&
                                 !strcmp(report.value[REPORT_RSS_STATE], "query_failed"));
            snprintf(text, sizeof text, "%s", failed_query ? "Query failed" : "Not measured");
        } else if (metrics[i].format == 1) {
            snprintf(text, sizeof text, value > 0 && value < 0.01 ? "%.6f ms" : "%.2f ms", value);
        } else if (metrics[i].format == 2) {
            snprintf(text, sizeof text, "%.2f MiB", value / 1048576.0);
        } else {
            snprintf(text, sizeof text, "%.100s", raw);
        }
        tether_ui_set_text(values[i], text);
    }
    if (strcmp(status, "passed")) {
        snprintf(text, sizeof text, "%.511s", report.value[REPORT_SUMMARY]);
    } else {
        const char *base = path;
        for (const char *p = path; *p; p++) {
            if (*p == '/' || *p == '\\') {
                base = p + 1;
            }
        }
        snprintf(text, sizeof text, "Loaded: %.180s", base);
    }
    tether_ui_set_text(report_label, text);
    return true;
}

bool results_load(const char *path) {
    return results_load_after_process(path, -1);
}
bool results_has_geometry(void) {
    Tether_SlotTransform *view = tether_ecs_get_component(tether_registry_find_entity("LabRoot"),
                                                          TETHER_COMPONENT_SLOT_TRANSFORM);
    if (!view) {
        return false;
    }
    /* Verify template parameters produce distinct card and value colors. */
    Tether_Shape *first_card = tether_ecs_get_component(cards[0], TETHER_COMPONENT_SHAPE);
    Tether_Shape *work_card = tether_ecs_get_component(cards[4], TETHER_COMPONENT_SHAPE);
    Tether_Text *first_value = tether_ecs_get_component(values[0], TETHER_COMPONENT_TEXT);
    Tether_Text *work_value = tether_ecs_get_component(values[4], TETHER_COMPONENT_TEXT);
    if (!first_card || !work_card || !first_value || !work_value ||
        !memcmp(&first_card->bg_color, &work_card->bg_color, sizeof(Tether_Color)) ||
        !memcmp(&first_value->color, &work_value->color, sizeof(Tether_Color))) {
        return false;
    }
    Tether_GUID labels[] = {report_label, title_label, import_label, status_label, scenario_label};
    for (unsigned i = 0; i < CARD_COUNT + sizeof labels / sizeof *labels; i++) {
        Tether_GUID id = i < CARD_COUNT ? values[i] : labels[i - CARD_COUNT];
        Tether_Text *text = tether_ecs_get_component(id, TETHER_COMPONENT_TEXT);
        uint8_t pipeline;
        uint32_t count = 0;
        if (!text || !chitra_table_get_vertices(text->handle, &pipeline, &count) || !count) {
            return false;
        }
        Tether_SlotTransform *position =
            tether_ecs_get_component(id, TETHER_COMPONENT_SLOT_TRANSFORM);
        if (!position || position->y < view->y ||
            position->y + text->intrinsic_size.y > view->y + view->height) {
            return false;
        }
        if (i < CARD_COUNT) {
            Tether_SlotTransform *bounds =
                tether_ecs_get_component(cards[i], TETHER_COMPONENT_SLOT_TRANSFORM);
            if (!bounds || bounds->width < 150 || bounds->height < 70 ||
                text->intrinsic_size.x > bounds->width - 20) {
                return false;
            }
        }
    }
    return true;
}
