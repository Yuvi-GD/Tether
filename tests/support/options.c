#include "tool.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool number(const char *text, uint32_t *out) {
    char *end;
    if (!text[0]) {
        return false;
    }
    for (const char *p = text; *p; p++) {
        if (*p < '0' || *p > '9') {
            return false;
        }
    }
    errno = 0;
    unsigned long long n = strtoull(text, &end, 10);
    if (errno || *end || n > UINT32_MAX) {
        return false;
    }
    *out = (uint32_t)n;
    return true;
}

int tool_parse_options(int argc, char **argv, bool test, ToolOptions *o) {
    *o = (ToolOptions){0};
    o->test = test;
    o->mode = "retained";
    o->output = NULL;
    o->count = 1000;
    o->frames = 120;
    o->warmup = test ? 0 : 30;
    o->seed = 1;
    o->memory_mib = 1024;
    o->fps = 60;
    if (argc == 1 || (argc == 2 && !strcmp(argv[1], "--help"))) {
        printf("Usage: %s --scenario %s [options]\n"
               "  --count N       Initial objects (10K/100K/1M/10M are workload choices)\n"
               "  --spawn N       Spawn this many objects per measured step\n"
               "  --delete N      Delete up to this many objects before spawning\n"
               "  --mutate N      Change up to this many live objects per step\n"
               "  --frames N      Measured steps; also the cap when --seconds is used\n"
               "  --seconds S     Optional measured wall-time limit\n"
               "  --warmup N      Render warmup frames, no workload mutations\n"
               "  --mode retained|volatile  Render update policy\n"
               "  --fps N         Window pacing: 0=VSync, >0=manual cap\n"
               "  --seed N        Deterministic color sequence\n"
               "  --memory-mib N  Conservative workload memory budget\n"
               "  --output PATH   JSON result; default is exe/report/<time>-<pid>.json\n"
               "No workload starts without --scenario. Closing a render window cancels the run.\n",
               argv[0], test ? "lifecycle|hierarchy|ui" : "ecs|render|layout");
        return 0;
    }
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            goto invalid;
        }
        const char *key = argv[i], *value = argv[i + 1];
        uint32_t *destination = NULL;
        if (!strcmp(key, "--scenario")) {
            o->scenario = value;
        } else if (!strcmp(key, "--mode")) {
            o->mode = value;
        } else if (!strcmp(key, "--output")) {
            o->output = value;
        } else if (!strcmp(key, "--count")) {
            destination = &o->count;
        } else if (!strcmp(key, "--spawn")) {
            destination = &o->spawn;
        } else if (!strcmp(key, "--delete")) {
            destination = &o->remove;
        } else if (!strcmp(key, "--mutate")) {
            destination = &o->mutate;
        } else if (!strcmp(key, "--frames")) {
            destination = &o->frames;
        } else if (!strcmp(key, "--warmup")) {
            destination = &o->warmup;
        } else if (!strcmp(key, "--fps")) {
            destination = &o->fps;
        } else if (!strcmp(key, "--seed")) {
            destination = &o->seed;
        } else if (!strcmp(key, "--memory-mib")) {
            destination = &o->memory_mib;
        } else if (!strcmp(key, "--seconds")) {
            char *end;
            errno = 0;
            o->seconds = strtod(value, &end);
            if (errno || end == value || *end || !isfinite(o->seconds) || o->seconds <= 0 ||
                o->seconds > 86400) {
                goto invalid;
            }
        } else {
            goto invalid;
        }
        if (destination && !number(value, destination)) {
            goto invalid;
        }
    }
    if (!o->scenario || (o->output && !o->output[0]) || !o->frames || !o->memory_mib ||
        o->frames > 10000000 || o->warmup > 10000000 || o->fps > 10000 ||
        (strcmp(o->mode, "retained") && strcmp(o->mode, "volatile"))) {
        goto invalid;
    }
    if (test ? (strcmp(o->scenario, "lifecycle") && strcmp(o->scenario, "hierarchy") &&
                strcmp(o->scenario, "ui"))
             : (strcmp(o->scenario, "ecs") && strcmp(o->scenario, "render") &&
                strcmp(o->scenario, "layout"))) {
        goto invalid;
    }
    if (!strcmp(o->scenario, "hierarchy") && (o->spawn || o->remove || o->mutate)) {
        fprintf(stderr, "Hierarchy checks use --count children and --frames repetitions; "
                        "spawn/delete/mutate do not apply.\n");
        return -1;
    }
    return 1;
invalid:
    fprintf(stderr, "Invalid or incomplete options. Run --help.\n");
    return -1;
}

bool tool_preflight(const ToolOptions *o, uint32_t *capacity, char *error, size_t size) {
    uint64_t peak = o->count;
    /* The first step may delete fewer objects than requested. */
    uint64_t after_first = (o->count > o->remove ? o->count - o->remove : 0) + (uint64_t)o->spawn;
    if (o->spawn > o->remove) {
        after_first += (uint64_t)(o->spawn - o->remove) * (o->frames - 1);
    }
    if (after_first > peak) {
        peak = after_first;
    }
    bool render = !strcmp(o->scenario, "render") || !strcmp(o->scenario, "ui");
    /* Chitra's shape staging uses 4 x 40-byte vertices and uint32_t byte
       products. Leave 2x growth headroom; 10M remains representable. */
    uint64_t limit = render ? UINT32_MAX / (4u * 40u * 2u) : UINT32_MAX / 16 - 4;
    if (peak > limit) {
        snprintf(error, size, "Workload exceeds representable engine/renderer capacity.");
        return false;
    }
    uint64_t estimate =
        peak * (render ? 2048 : 256) + (uint64_t)o->frames * sizeof(ToolSample) + 64 * 1024 * 1024;
    uint64_t budget = (uint64_t)o->memory_mib * 1024 * 1024;
    uint64_t available = tool_available_memory();
    if (estimate > budget || (available && estimate > available / 4 * 3)) {
        snprintf(error, size,
                 "Estimated workload needs %llu MiB; budget is %u MiB. Reduce the workload or "
                 "explicitly raise --memory-mib (available RAM also checked).",
                 (unsigned long long)((estimate + 1048575) / 1048576), o->memory_mib);
        return false;
    }
    *capacity = (uint32_t)(peak ? peak : 1);
    return true;
}
