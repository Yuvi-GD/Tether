#include "tool.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
    ToolOptions options;
    int parsed = tool_parse_options(argc, argv, TOOL_IS_TEST != 0, &options);
    if (parsed <= 0) {
        return parsed < 0 ? 2 : 0;
    }
    char default_output[4096];
    if (!options.output) {
        char directory[4096];
        if (!tool_executable_directory(directory, sizeof directory) ||
            snprintf(default_output, sizeof default_output, "%s/report/%lld-%u.json", directory,
                     (long long)time(NULL), tool_process_id()) >= (int)sizeof default_output) {
            fprintf(stderr, "Cannot determine the executable report directory.\n");
            return 2;
        }
        options.output = default_output;
    }
    if (!tool_make_output_directory(options.output)) {
        fprintf(stderr, "Cannot create the output directory for %s\n", options.output);
        return 2;
    }
    ToolResult result = {0};
    result.status = "running";
    result.backend = "none";
    result.cpu_ms = NAN;
    result.rss_start = result.rss_end = result.rss_peak = UINT64_MAX;
    snprintf(result.message, sizeof result.message, "Run started; no completed result yet.");
    if (!tool_write_result(&options, &result)) {
        return 2;
    }
    result.status = "failed";
    int code = tool_run(&options, &result);
    if (!tool_write_result(&options, &result)) {
        code = 2;
    }
    printf("%s\nResult: %s\n", result.message, options.output);
    char report[4096], text_path[4096];
    if (tool_read_report(options.output, report, sizeof report)) {
        printf("%s", report);
        if (snprintf(text_path, sizeof text_path, "%s.txt", options.output) >=
            (int)sizeof text_path) {
            code = 2;
        } else {
            FILE *f = fopen(text_path, "wb");
            if (!f) {
                code = 2;
            } else {
                bool ok = fputs(report, f) >= 0;
                if (fclose(f)) {
                    ok = false;
                }
                if (!ok) {
                    code = 2;
                }
            }
        }
    } else {
        code = 2;
    }
    free(result.samples);
    return code;
}
