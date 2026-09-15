#include "tether/tether.h"

int main() {
    Tether_App_Config config = {
        .width = 1024,
        .height = 768,
        .title = "Tether UI Sandbox",
        .initial_yaml = "../../examples/UI/index.yaml",
        
        /* Explicitly define our engine behavior */
        .memory_strategy = TETHER_MEMORY_HIGH_WATER_MARK,
        .renderer = TETHER_RENDERER_WEBGPU,
        .target_fps = 0 /* VSync */
    };
    tether_font_register("Roboto-Regular", "../../examples/Font/Roboto-Regular.ttf");
    
    tether_run(&config);
    return 0;
}