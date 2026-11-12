#ifndef SANDBOX_LAB_H
#define SANDBOX_LAB_H

#include "tether/core/tether_ecs.h"

bool lab_bind(void);
void lab_update(void);
void lab_cleanup(void);
bool lab_has_geometry(void);
void lab_smoke_launch(void);
void lab_smoke_detach(void);
int lab_smoke_status(void);

#endif
