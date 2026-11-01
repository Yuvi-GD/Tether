#ifndef SANDBOX_APP_H
#define SANDBOX_APP_H

#include "tether/core/tether_ecs.h"

typedef enum {
    SANDBOX_LAB,
    SANDBOX_SHOWCASE,
    SANDBOX_RESULTS
} SandboxView;

int sandbox_run(int argc, char **argv);
void sandbox_navigate(SandboxView view);
Tether_GUID sandbox_showcase_parent(void);
void sandbox_mount_scene(Tether_GUID root);

#endif
