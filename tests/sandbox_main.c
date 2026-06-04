#include "tether.h"

int main() {
    Tether_App_Config config = {
        .width = 1024,
        .height = 768,
        .title = "Tether UI Sandbox"
    };
    
    tether_run(&config);
    return 0;
}