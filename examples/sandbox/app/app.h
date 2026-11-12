#ifndef SANDBOX_APP_H
#define SANDBOX_APP_H

typedef enum {
    SANDBOX_WIDGETS = 0,
    SANDBOX_LAB,
    SANDBOX_SHOWCASE,
    SANDBOX_RESULTS
} SandboxView;

void sandbox_navigate(SandboxView view);

#endif
