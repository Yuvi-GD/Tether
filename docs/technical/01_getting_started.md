# Getting Started and Examples

[Documentation Master Index](../index.md) | [Next: Chapter 02 - Tether Core Architecture and Execution Model ->](02_core_architecture.md)

This guide walks you through building Tether from source, running the interactive sandbox showcase, and creating your first complete user interface using both **pure C** and **declarative YAML**.

---

## 1. Prerequisites and Toolchain

The reference build environment is **Windows x64** with:
- **CMake 3.20** or newer.
- **Visual Studio 2022** (with the *Desktop development with C++* workload installed).
- **Git** with submodule support.
- A functional graphics driver with modern GPU hardware acceleration support.

Tether core sources are written in **standard C99** for maximum portability, cache efficiency, and zero runtime bloat.

---

## 2. Cloning the Repository

Tether relies on two pinned submodules: **Chitra** (independent 2D GPU rendering engine) and **libyaml** (declarative parser). Always clone recursively:

```powershell
git clone --recurse-submodules https://github.com/Yuvi-GD/Tether.git
cd Tether
```

If you already cloned the repository without submodules, initialize them manually:

```powershell
git submodule update --init --recursive
```

---

## 3. Building and Running the Sandbox Showcase

The bundled **Tether Sandbox** provides an interactive showcase, widget gallery, YAML playground, and a Lab for launching workloads and reviewing reports. Refresh and playground actions reload YAML; there is no automatic file watcher.

### Step 1: Configure CMake
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

### Step 2: Build the Sandbox Target
```powershell
cmake --build build --config Release --target tether_sandbox --parallel 4
```

### Step 3: Run the Executable
Binaries are placed into configuration-isolated subdirectories (`build/<Config>/bin`):
```powershell
# Windows (Release or Debug)
.\build\Release\bin\tether_sandbox.exe
# Or Debug:
.\build\Debug\bin\tether_sandbox.exe

# Linux / macOS (or Ninja single-config)
./build/Release/bin/tether_sandbox
```

To jump directly into the YAML and C component showcase:
```powershell
.\build\Release\bin\tether_sandbox.exe --showcase
```

> [!TIP]
> **Asset Root Path Rule**: In C code and declarative YAML files, asset references are specified relative to the asset root without prefixing `assets/` (e.g. `"UI/app.yaml"`, `"Font/Roboto-Regular.ttf"`). The engine resolves relative assets automatically.

### Step 4: Verify the Build (Automated Test Suite)
Run all unit correctness and integration tests using CTest:
```powershell
ctest --test-dir build -C Release --output-on-failure
```

---

## 4. Building for WebAssembly (WebGL2 and WebGPU)

Tether natively supports WebAssembly compilation via Emscripten, enabling browser execution with zero garbage collection overhead, High-DPI canvas scaling, and mobile touch gesture support. Both WebGL2 and modern WebGPU backends are fully supported.

### Prerequisites for Web
- **Emscripten SDK (emsdk)**: Version 3.1.x or 4.0+.
- **Ninja**: Fast build system for Emscripten toolchains.
- **Python 3.x**: For running a local HTTP test server (optional if using `emrun`).

Install and activate Emscripten in your terminal:
```powershell
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
.\emsdk install latest
.\emsdk activate latest
.\emsdk_env.ps1
```

### Option A: WebGL2 Backend (Default Web Target)
```powershell
emcmake cmake -B build/wasm-webgl -G Ninja -DCMAKE_BUILD_TYPE=Release -DCHITRA_BACKEND=GLES3
cmake --build build/wasm-webgl

# Quick test with emrun (auto-launches default browser):
emrun build/wasm-webgl/tether_sandbox.html

# Or serve via local HTTP server:
python -m http.server -d build/wasm-webgl 8080
```
Open `http://localhost:8080/tether_sandbox.html` in your browser.

### Option B: WebGPU Backend (Next-Gen GPU Target)
```powershell
emcmake cmake -B build/wasm-webgpu -G Ninja -DCMAKE_BUILD_TYPE=Release -DCHITRA_BACKEND=WGPU
cmake --build build/wasm-webgpu

# Quick test with emrun:
emrun build/wasm-webgpu/tether_sandbox.html

# Or serve via local HTTP server:
python -m http.server -d build/wasm-webgpu 8081
```
Open `http://localhost:8081/tether_sandbox.html` in a WebGPU-supported browser (Chrome 113+, Edge 113+, Firefox Nightly, Safari 18+).

> [!NOTE]
> **Mobile Touch Navigation**: Touch inputs are normalized to pointer events (`POINTER_DOWN`, `POINTER_MOVE`, `POINTER_UP`) with CSS `touch-action: none` suppressing browser scrolling and zoom gestures.

### Using Tether Web in Host CMake Projects
Tether provides `cmake/TetherWasm.cmake` with the `tether_configure_wasm_target()` helper. In your host `CMakeLists.txt`:
```cmake
include(cmake/TetherWasm.cmake)

add_executable(my_web_app main.c)
target_link_libraries(my_web_app PRIVATE tether_runtime)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    tether_configure_wasm_target(my_web_app
        PRELOAD_DIRS
            "${CMAKE_CURRENT_SOURCE_DIR}/assets@assets"
    )
endif()
```
This automatically links Tether's official full-viewport web shell (`web/shell.html`), configures memory growth, exports required runtime symbols, and packages virtual filesystem mounts.

---

## 5. Copy-Paste Example 1: Minimal Pure C Application

This complete, self-contained C program creates a window, constructs a reactive UI with a title and an interactive counter button, updates text state on click, and cleans up on shutdown.

### `main.c`
```c
#include "tether/tether.h"
#include "tether/core/tether_input.h"
#include "tether/core/tether_events.h"
#include "tether/core/tether_registry.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"

#include <stdio.h>
#include <stdbool.h>

static TUID g_app_root = TETHER_INVALID_TUID;
static TUID g_counter_label = TETHER_INVALID_TUID;
static unsigned int g_click_count = 0;
static bool g_update_pending = false;

/* Event Callback: Triggered when the user clicks the button */
static void on_button_clicked(TUID entity, const Tether_Event* event, void* user_data) {
    (void)entity;
    (void)event;
    (void)user_data;
    g_click_count++;
    g_update_pending = true;
}

/* Lifecycle Hook: Called once after the graphics engine initializes */
static void on_app_init(void) {
    /* 1. Register application font (searched relative to executable directory) */
    tether_font_register("Roboto-Regular", "Font/Roboto-Regular.ttf");

    /* 2. Create the root column container attached to the main scene root */
    g_app_root = tether_ui_panel(tether_ecs_get_main_root());
    tether_ui_set_container(g_app_root, TETHER_FLOW_COLUMN);
    tether_ui_set_gap(g_app_root, 0.0f, 16.0f);
    tether_ui_set_padding(g_app_root, 24.0f, 24.0f, 24.0f, 24.0f);
    tether_ui_set_color(g_app_root, (Tether_Color){24, 30, 42, 255}); /* Dark slate background */

    /* 3. Add a header text label */
    TUID header = tether_ui_text(g_app_root);
    tether_ui_set_text(header, "Welcome to Tether");
    tether_ui_set_text_size(header, 28.0f);
    tether_ui_set_text_color(header, (Tether_Color){240, 244, 250, 255});
    tether_ui_set_size(header, 0.0f, 40.0f);

    /* 4. Add dynamic counter text */
    g_counter_label = tether_ui_text(g_app_root);
    tether_ui_set_text(g_counter_label, "Button clicks: 0");
    tether_ui_set_text_size(g_counter_label, 20.0f);
    tether_ui_set_text_color(g_counter_label, (Tether_Color){160, 180, 210, 255});
    tether_ui_set_size(g_counter_label, 0.0f, 32.0f);

    /* 5. Add an interactive button with a click callback */
    TUID btn = tether_ui_button(g_app_root);
    tether_ui_set_button_label(btn, "Click Me");
    tether_ui_set_button_color(btn, (Tether_Color){45, 105, 215, 255}); /* Blue accent */
    tether_ui_set_size(btn, 160.0f, 48.0f);
    tether_ui_set_flex(btn, 0.0f); /* Keep fixed button size */
    tether_bind_event(btn, TETHER_EVENT_CLICK, on_button_clicked, NULL);

    /* 6. Realize the widget hierarchy into the render tree */
    tether_widget_show(g_app_root);
}

/* Per-Frame Update Hook: Handles state mutations before layout and render */
static void on_app_frame(void) {
    if (g_update_pending) {
        g_update_pending = false;
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Button clicks: %u", g_click_count);
        tether_ui_set_text(g_counter_label, buffer);
    }
}

/* Lifecycle Hook: Clean up resources before exit */
static void on_app_term(void) {
    if (tether_ecs_is_valid(g_app_root)) {
        tether_widget_destroy(g_app_root);
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Tether_Config config = {
        .width = 800,
        .height = 600,
        .title = "Tether - Pure C Getting Started",
        .target_fps = 60,
        .on_init = on_app_init,
        .on_frame = on_app_frame,
        .on_term = on_app_term
    };

    tether_run(&config);
    return 0;
}
```

---

## 6. Copy-Paste Example 2: Declarative YAML UI

Tether allows declaring UI structure, styles, and nesting declaratively in **YAML**. Both YAML and C construct the exact same runtime ECS components in memory.

### Step 1: Create `app.yaml`
```yaml
Panel:
  flow: COLUMN
  gap: [0, 16]
  padding: 24
  color: "#181e2a"
  children:
    - Text:
        string: "Declarative UI with Tether"
        font_size: 28
        color: "#f0f4fa"
        size_box: [0, 40]

    - Text:
        id: "status_label"
        string: "Status: Ready"
        font_size: 20
        color: "#a0b4d2"
        size_box: [0, 32]

    - Button:
        id: "action_btn"
        color: "#2d69d7"
        size_box: [180, 48]
        fill_ratio: 0
        content:
          - Text:
              string: "Toggle Status"
              font_size: 18
```

### Step 2: Load and Bind Events in `main_yaml.c`
```c
#include "tether/tether.h"
#include "tether/core/tether_registry.h"
#include "tether/core/tether_events.h"
#include "tether/engine/parser/tether_yaml.h"
#include "tether/ui/tether_ui.h"
#include "tether/ui/tether_widgets.h"

#include <stdbool.h>

static TUID g_scene_root = TETHER_INVALID_TUID;
static bool g_toggle_state = false;

static void on_toggle_clicked(TUID entity, const Tether_Event* event, void* user_data) {
    (void)entity;
    (void)event;
    (void)user_data;

    g_toggle_state = !g_toggle_state;
    TUID label = tether_registry_find_entity("status_label");
    if (label != TETHER_INVALID_TUID) {
        tether_ui_set_text(label, g_toggle_state ? "Status: Active" : "Status: Ready");
        tether_ui_set_text_color(label, g_toggle_state ? (Tether_Color){70, 200, 120, 255} : (Tether_Color){160, 180, 210, 255});
    }
}

static void on_yaml_init(void) {
    tether_font_register("Roboto-Regular", "Font/Roboto-Regular.ttf");

    /* 1. Parse and instantiate the YAML tree under the main root */
    g_scene_root = tether_yaml_load("app.yaml", tether_ecs_get_main_root());
    if (g_scene_root == TETHER_INVALID_TUID) {
        return;
    }

    /* 2. Bind application callbacks to entities defined with an 'id' in YAML */
    TUID action_btn = tether_registry_find_entity("action_btn");
    if (action_btn != TETHER_INVALID_TUID) {
        tether_bind_event(action_btn, TETHER_EVENT_CLICK, on_toggle_clicked, NULL);
    }

    /* 3. Realize the widget hierarchy into view */
    tether_widget_show(g_scene_root);
}

static void on_yaml_term(void) {
    if (tether_ecs_is_valid(g_scene_root)) {
        tether_widget_destroy(g_scene_root);
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Tether_Config config = {
        .width = 800,
        .height = 600,
        .title = "Tether - Declarative YAML Getting Started",
        .target_fps = 60,
        .on_init = on_yaml_init,
        .on_term = on_yaml_term
    };

    tether_run(&config);
    return 0;
}
```

---

## 7. Copy-Paste Host `CMakeLists.txt`

To embed Tether into your own CMake project, place Tether in your project tree (e.g. as a `git submodule` in `third_party/tether`) and use this `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_tether_app C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Disable Tether examples and tests when embedding as a subproject
set(TETHER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TETHER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TETHER_BUILD_RUNNERS OFF CACHE BOOL "" FORCE)

# Add Tether library target
add_subdirectory(third_party/tether)

# Create your application executable
add_executable(my_tether_app main.c)

# Link against the public Tether interface target
target_link_libraries(my_tether_app PRIVATE tether)

# Copy Font and YAML assets to output folder
add_custom_command(TARGET my_tether_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Font"
        "$<TARGET_FILE_DIR:my_tether_app>/Font"
)
```

---

## 8. Common Pitfalls & Troubleshooting

| Problem | Cause | Solution |
| :--- | :--- | :--- |
| **Linker error: missing submodules** | Submodules (`chitra`, `libyaml`) were not checked out. | Run `git submodule update --init --recursive`. |
| **Text does not render on screen** | Font was not registered before post-init or font file not found. | Register the font in `on_init` using `tether_font_register("Name", "path.ttf")` and ensure the TTF exists in the working directory. |
| **Widgets exist in ECS but are invisible** | The widget hierarchy was not realized or is collapsed. | Call `tether_widget_show(root)` on your top-level container after building it. |
| **Direct component mutation has no visual effect** | Retained rendering was not notified of the change. | Add `tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL)` or use `tether_ui_set_text` / `tether_ui_set_color`. |
| **Top-level container fills the entire screen** | Top-level children of `main_root` automatically receive viewport dimensions. | Place fixed-size widgets inside a parent `tether_ui_panel` or container. |
| **YAML fails to load** | File path is incorrect relative to executable directory or syntax error. | Ensure the working directory contains `app.yaml` and verify YAML syntax (indentation using spaces, no tabs). |

---

[Documentation Master Index](../index.md) | [Next: Chapter 02 - Tether Core Architecture and Execution Model ->](02_core_architecture.md)

