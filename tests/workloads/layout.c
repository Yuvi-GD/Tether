#include "tool.h"
#include "tether/core/tether_components.h"
#include "tether/core/tether_ecs.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_input.h"
#include "tether/core/tether_registry.h"
#include "tether/engine/tether_engine.h"
#include "tether/engine/tether_layout.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>


/* The timer surrounds only the existing Tether layout entry point. Structure
   creation, resize/invalidation, assertions and teardown are outside it. */
int tool_run_layout(const ToolOptions *o, ToolResult *r) {
    double overall_start = tool_now_ms();
    r->backend = "Tether layout";
    r->samples = calloc(o->frames, sizeof *r->samples);
    if (!r->samples) {
        r->status = "failed";
        r->termination_reason = "allocation_failure";
        snprintf(r->message, sizeof r->message, "Layout sample allocation failed.");
        return 2;
    }
    tether_init(NULL);
    Tether_GUID root = tether_widget_create_panel(tether_ecs_get_main_root());
    tether_ui_set_size(root, 800, 600);
    Tether_Layout *root_layout = tether_ecs_get_component(root, TETHER_COMPONENT_LAYOUT);
    root_layout->flow = TETHER_FLOW_COLUMN;
    uint32_t active = 1;
    Tether_GUID first_leaf = TETHER_INVALID_GUID;
    Tether_GUID row = TETHER_INVALID_GUID;
    double create_start = tool_now_ms();
    for (uint32_t i = 0; i < o->count; i++) {
        if (i % 25 == 0) {
            row = tether_widget_create_panel(root);
            if (!tether_ecs_is_valid(row)) {
                r->failures++;
                break;
            }
            Tether_Layout *layout = tether_ecs_get_component(row, TETHER_COMPONENT_LAYOUT);
            layout->flow = TETHER_FLOW_ROW;
            tether_ui_set_size(row, 0, 30);
            active++;
        }
        Tether_GUID leaf = tether_widget_create_panel(row);
        if (!tether_ecs_is_valid(leaf)) {
            r->failures++;
            break;
        }
        tether_ui_set_size(leaf, 20, 20);
        if (first_leaf == TETHER_INVALID_GUID) {
            first_leaf = leaf;
        }
        active++;
    }
    r->create_ms = tool_now_ms() - create_start;
    r->initial_active = r->final_active = r->peak_active = active;
    r->setup_ms = tool_now_ms() - overall_start;
    r->rss_start = tool_rss();
    r->rss_peak = r->rss_start;
    double cpu_start = tool_cpu_ms();
    double measured_start = tool_now_ms();
    if (!r->failures) {
        for (uint32_t step = 0; step < o->frames; step++) {
            float width = step % 2 ? 1000.0f : 800.0f;
            tether_ui_set_size(root, width, 600);
            if (first_leaf != TETHER_INVALID_GUID) {
                tether_ui_set_size(first_leaf, step % 2 ? 22.0f : 20.0f, 20);
            }
            double start = tool_now_ms();
            tether_layout_process_all(width, 600);
            ToolSample *sample = &r->samples[step];
            sample->work_ms = tool_now_ms() - start;
            sample->alive = active;
            sample->rss_bytes = tool_rss();
            r->layout_ms += sample->work_ms;
            r->checks++;
            Tether_SlotTransform *bounds =
                tether_ecs_get_component(root, TETHER_COMPONENT_SLOT_TRANSFORM);
            if (!bounds || fabsf(bounds->width - width) > 0.1f) {
                r->failures++;
            }
            if (sample->rss_bytes != UINT64_MAX &&
                (r->rss_peak == UINT64_MAX || sample->rss_bytes > r->rss_peak)) {
                r->rss_peak = sample->rss_bytes;
            }
            r->sample_count++;
            r->elapsed_ms = tool_now_ms() - measured_start;
            if (r->failures || (o->seconds > 0 && r->elapsed_ms >= o->seconds * 1000.0)) {
                break;
            }
        }
    }
    r->cpu_ms = tool_cpu_ms() - cpu_start;
    r->rss_end = tool_rss();
    if (r->rss_end != UINT64_MAX && (r->rss_peak == UINT64_MAX || r->rss_end > r->rss_peak)) {
        r->rss_peak = r->rss_end;
    }
    r->termination_reason = r->failures                    ? "assertion_failure"
                            : r->sample_count == o->frames ? "step_limit"
                                                           : "time_limit";
    r->status = r->failures ? "failed" : "passed";
    double shutdown_start = tool_now_ms();
    tether_widget_destroy(root);
    r->teardown_ms = tool_now_ms() - shutdown_start;
    tether_input_term();
    tether_events_term();
    tether_registry_term();
    tether_ecs_term();
    r->shutdown_ms = tool_now_ms() - shutdown_start;
    r->total_run_ms = tool_now_ms() - overall_start;
    snprintf(r->message, sizeof r->message,
             "layout: %s; %u measured passes, %u active panels, %.3f ms isolated layout time.",
             r->status, r->sample_count, active, r->layout_ms);
    return r->failures ? 1 : 0;
}
