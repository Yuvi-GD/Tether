#include "tool.h"
#include <string.h>

static const char *labels[REPORT_FIELD_COUNT] = {"Status",
                                                 "Result",
                                                 "Scenario",
                                                 "Mode",
                                                 "Initial objects",
                                                 "Measured steps",
                                                 "Total checks",
                                                 "Failed checks",
                                                 "Elapsed wall time (ms)",
                                                 "Process CPU time (ms)",
                                                 "Peak process RAM (sampled bytes)",
                                                 "Avg work / step (ms)",
                                                 "Median work (ms)",
                                                 "P95 work / step (ms)",
                                                 "GPU time",
                                                 "Draw submissions",
                                                 "Upload requests (bytes)",
                                                 "Kind",
                                                 "Avg frame interval (ms)",
                                                 "P95 frame interval (ms)",
                                                 "Isolated layout time (ms)",
                                                 "Process CPU measurement state",
                                                 "Resident memory measurement state"};

bool tool_read_report(const char *path, char *out, size_t size) {
    if (!size) {
        return false;
    }
    out[0] = 0;
    ToolReport report;
    if (!tool_read_report_data(path, &report)) {
        return false;
    }
    size_t used = 0;
    for (unsigned i = 0; i < REPORT_FIELD_COUNT; i++) {
        int count = snprintf(out + used, size - used, "%s: %s\n", labels[i],
                             report.value[i][0] ? report.value[i] : "unavailable");
        if (count < 0 || (size_t)count >= size - used) {
            return false;
        }
        used += (size_t)count;
    }
    return true;
}
