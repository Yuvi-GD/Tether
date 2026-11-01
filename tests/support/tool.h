#ifndef TETHER_TOOL_H
#define TETHER_TOOL_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    const char *scenario, *mode, *output;
    uint32_t count, spawn, remove, mutate, frames, warmup, seed, memory_mib, fps;
    double seconds;
    bool test;
} ToolOptions;

typedef struct {
    double work_ms, interval_ms, spawn_ms, delete_ms, mutate_ms;
    uint32_t alive, spawned, deleted, mutated, draws, viewport_width, viewport_height;
    uint64_t upload_bytes;
    uint64_t rss_bytes;
    uint32_t visible;
} ToolSample;

typedef struct {
    const char *status, *backend;
    char message[512];
    ToolSample *samples;
    uint32_t sample_count;
    uint64_t checks, failures, rss_start, rss_end, rss_peak;
    double create_ms, teardown_ms, elapsed_ms, cpu_ms;
    double setup_ms;
    double warmup_ms;
    double validation_ms;
    double shutdown_ms;
    double total_run_ms;
    double layout_ms;
    uint32_t warmup_frames;
    uint32_t initial_active;
    uint32_t final_active;
    uint32_t peak_active;
    const char *termination_reason;
} ToolResult;

/* Process handles and buffers are owned by the caller. No worker threads. */
typedef struct {
    void *handle;
    int pid;
    bool active;
} ToolProcess;
double tool_now_ms(void);
double tool_cpu_ms(void);
uint32_t tool_process_id(void);
uint64_t tool_rss(void);
uint64_t tool_available_memory(void);
void tool_environment(FILE *file);
bool tool_executable_directory(char *out, size_t size);
bool tool_enter_executable_directory(void);
bool tool_make_results_directory(const char *path);
bool tool_make_output_directory(const char *output);
bool tool_launch(ToolProcess *process, const char *name, const char *const *args);
bool tool_poll(ToolProcess *process, int *exit_code);
void tool_detach(ToolProcess *process);
void tool_json_string(FILE *file, const char *text);
int tool_parse_options(int argc, char **argv, bool test, ToolOptions *options);
bool tool_preflight(const ToolOptions *options, uint32_t *capacity, char *error, size_t size);
bool tool_write_result(const ToolOptions *options, const ToolResult *result);
bool tool_read_summary(const char *path, char *out, size_t size);
bool tool_read_report(const char *path, char *out, size_t size);
typedef enum {
    REPORT_STATUS,
    REPORT_SUMMARY,
    REPORT_SCENARIO,
    REPORT_MODE,
    REPORT_COUNT,
    REPORT_STEPS,
    REPORT_CHECKS,
    REPORT_FAILURES,
    REPORT_ELAPSED,
    REPORT_CPU,
    REPORT_RSS,
    REPORT_MEAN,
    REPORT_MEDIAN,
    REPORT_P95,
    REPORT_GPU,
    REPORT_DRAWS,
    REPORT_UPLOADS,
    REPORT_KIND,
    REPORT_FRAME_AVG,
    REPORT_FRAME_P95,
    REPORT_LAYOUT,
    REPORT_CPU_STATE,
    REPORT_RSS_STATE,
    REPORT_FIELD_COUNT
} ToolReportField;
/* Caller-owned snapshot; no sample array or pointers into parser storage. */
typedef struct {
    char value[REPORT_FIELD_COUNT][512];
} ToolReport;
bool tool_read_report_data(const char *path, ToolReport *report);
bool tool_pick_result(char *path, size_t size);
bool tool_replace_file(const char *source, const char *destination);
int tool_run(const ToolOptions *options, ToolResult *result);
#endif
