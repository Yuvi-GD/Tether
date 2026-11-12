#ifndef SANDBOX_RESULTS_H
#define SANDBOX_RESULTS_H

#include <stdbool.h>
#include "tether/core/tether_ecs.h"

bool results_bind(void);
bool results_load(const char *path);
bool results_load_after_process(const char *path, int exit_code);
void results_message(const char *message);
bool results_has_geometry(void);

#endif
