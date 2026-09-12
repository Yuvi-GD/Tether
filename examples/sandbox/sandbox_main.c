#include "tether/tether.h"

int main() {
    Tether_App_Config config = {
        .width = 1024,
        .height = 768,
        .title = "Tether UI Sandbox",
        .initial_yaml = "index.yaml"
    };
    
    tether_run(&config);
    return 0;
}