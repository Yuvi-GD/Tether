#include "tool.h"
#include <math.h>
#include <string.h>

static unsigned failures;
static const char *one_sample = "[{\"step\":0,\"alive\":100,\"work_ms\":0.5}]";

static void check(bool condition, const char *description) {
    if (!condition) {
        fprintf(stderr, "FAILED: %s\n", description);
        failures++;
    }
}

static bool write_fixture(const char *path, const char *status, unsigned failed_checks,
                          unsigned steps, const char *samples, const char *suffix) {
    FILE *file = fopen(path, "wb");
    if (!file) {
        return false;
    }
    int written =
        fprintf(file,
                "{\"schema_version\":1,\"project\":\"Tether\",\"kind\":\"correctness\","
                "\"status\":\"%s\",\"summary\":\"fixture\","
                "\"parameters\":{\"scenario\":\"ui\",\"mode\":\"retained\",\"count\":100},"
                "\"metrics\":{\"checks\":10,\"failures\":%u,\"measured_steps\":%u,\"gpu_ms\":null},"
                "\"samples\":%s%s",
                status, failed_checks, steps, samples, suffix);
    bool ok = written >= 0;
    if (fclose(file)) {
        ok = false;
    }
    return ok;
}

static void expect_fixture(const char *path, const char *status, unsigned failed_checks,
                           unsigned steps, const char *samples, const char *suffix, bool expected,
                           const char *description) {
    check(write_fixture(path, status, failed_checks, steps, samples, suffix), "write fixture");
    ToolReport report;
    check(tool_read_report_data(path, &report) == expected, description);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return 2;
    }
    const char *path = argv[1];
    expect_fixture(path, "passed", 0, 1, one_sample, "}", true, "complete report");
    ToolReport report;
    check(tool_read_report_data(path, &report), "read structured values");
    check(!strcmp(report.value[REPORT_COUNT], "100"), "object count");
    check(!strcmp(report.value[REPORT_CHECKS], "10"), "total checks");
    check(!report.value[REPORT_GPU][0], "null GPU time remains unavailable");
    check(!report.value[REPORT_DRAWS][0], "older reports may omit aggregate counters");
    char text[4096];
    check(tool_read_report(path, text, sizeof text), "human report formatting");
    check(strstr(text, "GPU time: unavailable") != NULL, "human unavailable value");

    expect_fixture(path, "passed", 0, 1, one_sample, "", false, "truncated after samples");
    expect_fixture(path, "passed", 0, 1, "[", "", false, "truncated inside samples");
    expect_fixture(path, "passed", 2, 1, one_sample, "}", false, "success contradicts failures");
    expect_fixture(path, "passed", 0, 2, one_sample, "}", false, "missing measured sample");
    expect_fixture(path, "passed", 0, 0, "[]", "}", false, "zero-step success rejected");
    expect_fixture(path, "passed", 0, 1, "42", "}", false, "sample array required");
    expect_fixture(path, "passed", 0, 1, one_sample, "} junk", false, "trailing garbage");
    expect_fixture(path, "passed", 0, 1, one_sample, ",\"status\":\"passed\"}", false,
                   "duplicate completion status");
    expect_fixture(path, "passed", 0, 1, one_sample,
                   ",\"measurement_state\":{\"process_cpu\":\"measured\"}}", false,
                   "measured CPU state requires a value");
    expect_fixture(path, "running", 0, 1, one_sample, "}", false, "unfinished run");
    expect_fixture(path, "cancelled", 0, 1, one_sample, "}", true,
                   "cancelled run remains readable");
    expect_fixture(path, "failed", 2, 1, one_sample, "}", true, "failed run remains readable");
    expect_fixture(path, "rejected", 0, 0, "[]", "}", true, "preflight rejection remains readable");
    check(remove(path) == 0, "remove fixture");
    check(!tool_read_report_data(path, &report), "missing file");
    check(!report.value[REPORT_COUNT][0], "failed import clears old values");
    ToolSample sample = {.alive = 1, .work_ms = 0.5, .rss_bytes = UINT64_MAX};
    ToolResult result = {.status = "passed",
                         .backend = "none",
                         .samples = &sample,
                         .sample_count = 1,
                         .cpu_ms = NAN,
                         .rss_start = UINT64_MAX,
                         .rss_end = UINT64_MAX,
                         .rss_peak = UINT64_MAX,
                         .termination_reason = "step_limit"};
    ToolOptions options = {.scenario = "ecs", .mode = "retained", .output = path, .count = 1};
    check(tool_write_result(&options, &result), "write unavailable counters");
    check(tool_read_report_data(path, &report), "read unavailable counters");
    check(!report.value[REPORT_CPU][0] && !report.value[REPORT_RSS][0],
          "failed queries are not zero measurements");
    check(!strcmp(report.value[REPORT_CPU_STATE], "query_failed") &&
              !strcmp(report.value[REPORT_RSS_STATE], "query_failed"),
          "failed counter state is explicit");
    result.cpu_ms = 0;
    result.rss_start = result.rss_end = result.rss_peak = sample.rss_bytes = 0;
    check(tool_write_result(&options, &result), "write valid zero counters");
    check(tool_read_report_data(path, &report), "read valid zero counters");
    check(!strcmp(report.value[REPORT_CPU], "0.000000") && !strcmp(report.value[REPORT_RSS], "0"),
          "valid zero remains distinguishable from unavailable");
    check(remove(path) == 0, "remove generated report fixture");
    printf("Report import: %s\n", failures ? "FAILED" : "PASSED");
    return failures ? 1 : 0;
}
