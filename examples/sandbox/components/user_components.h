#ifndef USER_COMPONENTS_H
#define USER_COMPONENTS_H

#include "tether/core/tether_components.h"
#include "tether/core/tether_ecs.h"

#undef TETHER_COMPONENT_MAX

// example of user component
// typedef struct {
//     float current;
//     float maximum;
// } Game_Health;

// typedef struct {
//     float rate;
// } Rate;


// enum {
//     GAME_COMPONENT_HEALTH = TETHER_SYSTEM_COMPONENTS_MAX,
//     GAME_COMPONENT_RATE,
//     TETHER_COMPONENT_MAX
// };

// if you have list don't define TETHER_COMPONENT_MAX and keep it last in list.
// TETHER_COMPONENT_MAX define the limit of defined component limit that created in compiletime,
// so dynamic(runtime) component can get the starting index and it doesn't waste random index memeory.
enum{
    TETHER_COMPONENT_MAX = TETHER_SYSTEM_COMPONENTS_MAX
};

#endif /* USER_COMPONENTS_H */

