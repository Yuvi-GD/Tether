<div align="center">

# Tether UI Engine
### **TETHER** : TETHER - ECS Toolkit for High-Efficiency Runtime

[![Tether Correctness CI](https://github.com/Yuvi-GD/Tether/actions/workflows/ci.yml/badge.svg)](https://github.com/Yuvi-GD/Tether/actions/workflows/ci.yml)
[![Tether WebAssembly CI](https://github.com/Yuvi-GD/Tether/actions/workflows/wasm.yml/badge.svg)](https://github.com/Yuvi-GD/Tether/actions/workflows/wasm.yml)
[![Documentation](https://img.shields.io/badge/docs-reference-blue.svg)](docs/index.md)
[![License](https://img.shields.io/badge/license-Apache%202.0-2ea44f)](LICENSE)
![Language](https://img.shields.io/badge/language-C99-orange)

**Major Platforms:**
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)
![macOS](https://img.shields.io/badge/macOS-000000?logo=apple&logoColor=white)
![Android](https://img.shields.io/badge/Android-3DDC84?logo=android&logoColor=white)
![iOS](https://img.shields.io/badge/iOS-000000?logo=apple&logoColor=white)
![WebAssembly](https://img.shields.io/badge/WebAssembly-654FF0?logo=webassembly&logoColor=white)

*Tether is designed for portable C environments. Other targets may require a custom backend, platform integration, or additional work.*

[**Quick Start**](#10-building--running) &nbsp;|&nbsp;
[**Architecture**](#3-cell-block-subsystem-architecture) &nbsp;|&nbsp;
[**Platform Matrix**](#5-platform-support--sdk-distribution) &nbsp;|&nbsp;
[**Widget Suite**](#6-built-in-widget-suite) &nbsp;|&nbsp;
[**Docs Hub**](docs/index.md) &nbsp;|&nbsp;
[**Contributing**](CONTRIBUTING.md) &nbsp;|&nbsp;
[**Releases**](https://github.com/Yuvi-GD/Tether/releases)

</div>

Tether is a high-performance, modular user interface engine written in pure ISO C99. Engineered for predictable latency, cache locality, and zero garbage collection overhead, Tether replaces heavy DOM trees and deeply nested object-oriented inheritance hierarchies with a data-oriented Entity Component System (UI-ECS) and a reactive two-pass layout solver. By separating persistent UI layout space from presentation transforms, Tether ensures that animations, state changes, and styling updates execute with minimal CPU reflow and deterministic frame times.

Tether is distributed as a **source SDK** and separate interactive demo packages. Tether's C API is intended for use beyond the platforms listed here, including with custom backends. The supplied build pairs it with **Chitra**, whose Sokol backends target Windows, Linux, macOS, iOS, Android, and WebAssembly. Other environments require suitable platform integration and validation; a C compiler alone does not guarantee the default build will run.

---

<img alt="Tether showcase" src="https://github.com/user-attachments/assets/3f442144-2c24-4982-9859-c00c07ae9607" width="100%" />

---
## 1. Architectural Vision: The "PDF of Interactive User Interfaces"

Digital computing solved media portability decades ago by establishing open, vendor-neutral file specifications:
- **Audio has MP3 / FLAC**: Plays consistently on any speaker, smartphone, or dashboard, isolated from DAC hardware or audio drivers.
- **Images have PNG / JPEG**: Renders the exact same pixels across all viewers, isolated from graphics cards.
- **Documents have PDF (Portable Document Format)**: Preserves typography, spatial layout, and vector curves identically across all operating systems and printers.

**User interfaces have historically lacked this universal standard.**

For decades, graphical interfaces have remained trapped inside vendor-locked compilers (Win32, Cocoa, Qt), proprietary game engine codebases, or bloated multi-hundred-megabyte browser engines (Chromium/Electron).

Tether's long-term vision is a portable, open way to describe interactive interfaces. A future bytecode format (`.tbc`) and support for the following environments are upcoming roadmap targets:
- An automotive touchscreen or medical monitor running an RTOS,
- A bare-metal microcontroller driving an SPI or parallel LCD,
- A modern web browser executing WebAssembly via WebGPU or Canvas,
- A high-end desktop workstation running DirectX 11, Metal, or Vulkan,
- An AAA game engine viewport in Unreal, Unity, or custom engines.

---

## 2. Core Architectural Highlights

Tether abandons the traditional Document Object Model (DOM), virtual DOM diffing, and deeply nested C++ object-oriented inheritance trees in favor of six foundational systems-level properties:

### 1. Retained State and Invalidation
Tether retains UI state and uses dirty tags (`DIRTY_LAYOUT`, `DIRTY_VISUAL`, `DIRTY_HIERARCHY`) to identify changes. Volatile entities can also be updated continuously. Idle CPU and GPU usage depend on the application and backend and have not been established as zero.

### 2. Data-Oriented UI-ECS
UI components are stored in type-specific dense arrays with sparse lookup. Memory use depends on the scene, component capacities, text, and backend resources; embedded memory targets remain to be measured.

### 3. Managed Component Storage
Component pools grow as needed, and text may allocate when its contents change. Reducing runtime allocations is a goal, not a guarantee for all operations.

### 4. Separation of Layout Space from Render Space
Tether separates layout geometry (`Tether_SlotTransform`) from presentation (`Tether_RenderTransform`). Some visual updates can avoid layout; text transform changes may rebuild glyph geometry and are not GPU-only operations.

### 5. Universal 64-Bit Generational Handles (`TUID`)
Entity identifiers contain an index and generation, and `tether_ecs_is_valid()` checks their validity. Component APIs also expose pointers; callers must reacquire them after mutations that can move component storage.

### 6. YAML and C Authoring
Declarative YAML and C can use the same registered widget factories:
- Interfaces declared in YAML and interfaces declared imperatively via the fluent `tether_ui` API instantiate the same underlying ECS components.
- The Sandbox supports manual UI refresh and reloading declarative YAML scenes during development (by rebuilding the active subtree).

---

## 3. Cell-Block Subsystem Architecture

Tether is organized into autonomous functional cells that communicate through explicit, minimal C interface contracts:

```mermaid
flowchart TD
    subgraph HostDomain["1. Application & Authoring Layer"]
        Host["Host Application (C99, C++, Rust, Python, WASM)"]
        Authoring["Authoring Frontends (Declarative YAML, Pure C99 API, Scene AST)"]
        UIFramework["UI Framework & Registries (Panel, Button, Text, Aliases)"]
    end

    subgraph CoreDomain["2. Tether Core Engine (Pure C99 Micro-Kernel)"]
        Engine["Engine Orchestrator (Transforms, Invalidation, Roots, Events)"]
        Kernel["ECS Memory Kernel (Sparse Sets & Contiguous Dense Arrays)"]
    end

    subgraph Solvers["3. Layout Subsystem"]
        LayoutInterface["Built-in Layout Processing"]
        LayoutEngine["Layout Engine (Two-Pass Measure/Arrange, Anchor/Flex)"]
    end

    subgraph Drivers["4. Hardware & Graphics Subsystems"]
        BackendContract["Tether_Backend Contract Table (System, Shape, Text)"]
        Chitra["Chitra GPU Engine (SDF Shaders, Glyph Cache, Sokol/SDL)"]
        AlternativeDrivers["Alternative Backends (ThorVG, Raylib, Bare-Metal LCDs)"]
    end

    Host --> UIFramework
    Authoring --> UIFramework
    UIFramework --> Engine
    Engine --> Kernel
    Engine --> LayoutInterface
    LayoutInterface --> LayoutEngine
    Engine --> BackendContract
    BackendContract --> Chitra
    BackendContract -.-> AlternativeDrivers
```

---

## 4. Rendering Pipeline: Chitra

Tether isolates all hardware rasterization behind an abstract Render Hardware Interface (RHI). Default 2D GPU rendering is driven by **[Chitra](https://github.com/Yuvi-GD/Chitra)**:

* **Retained Command Table:** UI primitives are tracked in contiguous 16-byte command slots with 64-bit visual sort keys.
* **Radix Sort Z-Banding:** Coalesces draw calls by pipeline state, scissor clip regions, and texture bindings using high-speed LSD Radix Sort.
* **SDF Primitives:** Draws rounded rectangles and borders with a shape shader.
* **Dynamic Glyph Atlas:** Integrates Fontstash for high-performance glyph rasterization and texture caching.
* **Hardware Scissor Clipping:** Nested clipping regions are calculated hierarchically and submitted directly to hardware scissor registers.
* **Driver-Agnostic Abstraction:** Chitra supports multiple low-level backends (DirectX 11, Apple Metal, OpenGL Core, OpenGL ES) through Sokol.

---

## 5. Platform Support & SDK Distribution

Tether Core is pure standard C99 with zero operating-system or GPU dependencies, allowing it to run anywhere C can run. Tether's architecture cleanly separates the core UI engine from platform windowing and hardware rendering via the `Tether_Backend` interface table (`system`, `shape`, `text`).

### Platform Deployment Matrix

| Platform / Target | Default Setup (Chitra + Sokol) | Custom Backend (`Tether_Backend`) | Implementation / Driver Details |
| :--- | :---: | :---: | :--- |
| **Windows 10 / 11** | Supported | Supported | Direct3D 11 / OpenGL Core via Sokol (CI validated) |
| **Linux (X11 / Wayland)** | Supported | Supported | OpenGL Core 3.3 via Sokol (CI validated), custom DRM/KMS or SDL3 |
| **macOS (10.13+)** | Supported | Supported | Apple Metal via Sokol (CI validated) |
| **Web (Browsers)** | Supported | Supported | WebAssembly via Emscripten: WebGL2 & WebGPU with High-DPI & mobile touch (CI validated) |
| **Android** | Supported | Supported | OpenGL ES 3.0 via Sokol / Android NDK |
| **iOS / iPadOS** | Supported | Supported | Apple Metal via Sokol / UIKit |
| **Embedded Linux / SBC** | Supported | Supported | OpenGL ES 2.0/3.0 via Sokol, or direct DRM/KMS framebuffer |
| **Game Engines (Unreal, Unity, Godot)** | Supported | Supported | Offscreen GPU texture rendering or engine RHI; driven via `tether_on_*` frame hooks |
| **RTOS (FreeRTOS, Zephyr, NuttX)** | [Roadmap] | Supported | Pure C99 core links directly to RTOS tasks; draws to LCD display controller |
| **Microcontrollers (Cortex-M, ESP32, RISC-V)** | [Roadmap] | Supported | Sub-64 KB static SRAM target using pure C mode with line-buffer / dirty-tile blitting |

> [!NOTE]
> **Tether is Decoupled from Chitra**: Chitra is Tether's default, reference 2D GPU backend. Developers can replace Chitra at link time with any custom backend (e.g. ThorVG, Skia, Raylib, SDL3, or a raw CPU software blitter) via the `Tether_Backend` contract table (`.system`, `.shape`, `.text`) without modifying a single line of Tether Core.

### SDK & Distribution Model

* **Source SDK**: Tether source and public headers, the default Chitra backend, libyaml, generated shader headers, licenses, and CMake build files. Platform toolchains and graphics dependencies are required.
* **Showcase Demos**: Standalone interactive showcase binaries (`tether_sandbox`) distributed separately for evaluating UI widgets, live YAML authoring, and benchmark telemetry without compiling source code.

### Game Engine & Embedded Integration Models

* **Game Engine Viewports**: Game engines do not yield their window loops to external libraries. Tether integrates into custom loops via lifecycle hooks (`tether_on_init`, `tether_on_input`, `tether_on_frame`, `tether_on_cleanup`). The engine renders into an offscreen GPU texture target that the game composites into HUD overlays or in-game 3D world-space surfaces (diegetic computer terminals, vehicle dashboards).
* **Microcontroller SRAM Footprint**: Tether Core requires under 15 KB of static RAM for scenes with 50-80 UI entities. For displays on microcontrollers with 64 KB total SRAM, the software backend uses banded line-buffering (e.g. 16-32 scanline strips) or dirty-rectangle blitting directly to display controller GRAM (e.g. ST7789, ILI9341 via SPI DMA), eliminating the need for a full uncompressed framebuffer in internal RAM.

---

## 6. Built-in Widget Suite

Tether ships with built-in widgets and semantic state components (`Tether_ValueRange`, `Tether_BoolState`, `Tether_SelectionState`). YAML exposes a subset of their C API properties:

| Category | Built-in Widgets |
| :--- | :--- |
| **Primitives & Containers** | `Container`, `Panel`, `Text`, `tether_list` |
| **Actions & Inputs** | `Button`, `Toggle`, `Checkbox`, `Radio`, `Slider`, `SegmentedBar` |
| **Navigation & Disclosure** | `Menu`, `Dropdown`, `ContextMenu`, `Collapsible`, `Switcher` |
| **Feedback & Status** | `ProgressBar`, Disabled states (`tether_widget_set_enabled`) |

Overlays (`Dropdown`, `Menu`, `ContextMenu`) use CSS-like `TETHER_POSITION_ABSOLUTE` positioning with single-pass draw-order layering (`N++`), escaping parent clip bounds cleanly without reparenting or z-index hacks. Right-click pointer events bubble natively up the hierarchy, and drag-to-select + click-outside dismissal are handled via unified input routing.

---

## 7. Documentation Hub

Comprehensive engineering and architectural documentation is available in the [`docs/`](docs/) directory:

* **[Master Documentation Index](docs/index.md)**: Master navigation hub and ecosystem architecture overview.
* **[Category A: Project Presentation (10 Chapters)](docs/index.md#category-a-project-presentation)**: Vision, design philosophy, ecosystem architecture, technology comparisons, benchmark methodology, and phased roadmap.
* **[Category B: Technical & Developer Documentation (14 Chapters)](docs/index.md#category-b-technical-and-developer-documentation)**: Complete engineering manuals covering the ECS Kernel, memory layout, components, invalidation lifecycles, widget registries, layout engines, backend interfaces, GPU rendering, input hit-testing, declarative parsing, language bindings, API reference, benchmarks, and architectural decision records (ADRs).

---

## 8. Dependencies

Tether core requires zero dynamic external runtime libraries beyond the standard C library. The build vendors two submodule dependencies:

| Dependency | Location | Purpose |
| :--- | :--- | :--- |
| **[Chitra](https://github.com/Yuvi-GD/Chitra)** | `third_party/chitra` | Retained-mode 2D GPU rendering engine and windowing abstraction |
| **[libyaml](https://github.com/yaml/libyaml)** | `third_party/libyaml` | Fast C YAML parsing for declarative layout definitions |

---

## 9. Prerequisites

* **CMake:** Version 3.20 or newer across all platforms.
* **Windows:** Visual Studio 2022 (MSVC with "Desktop development with C++") or LLVM Clang.
* **Linux:** GCC or Clang, with development packages for X11 and OpenGL:
  * Ubuntu / Debian: `sudo apt install -y build-essential cmake libx11-dev libxi-dev libxcursor-dev libgl1-mesa-dev`
  * Fedora / RHEL: `sudo dnf install -y gcc gcc-c++ cmake libX11-devel libXi-devel libXcursor-devel mesa-libGL-devel`
  * Arch Linux: `sudo pacman -S --needed base-devel cmake libx11 libxi libxcursor mesa`
* **macOS:** Xcode Command Line Tools (`xcode-select --install`) and CMake (`brew install cmake`).

---

## 10. Building & Running

### 1. Clone with Submodules
```bash
git clone --recurse-submodules https://github.com/Yuvi-GD/Tether.git
cd Tether
```

If already cloned without submodules, initialize them:
```bash
git submodule update --init --recursive
```

### 2. Configure & Build

#### Multi-Configuration Generators (Visual Studio / MSVC)
```bash
# Configure
cmake -B build

# Build in Release configuration
cmake --build build --config Release
```

#### Single-Configuration Generators (Ninja / Makefiles / GCC / Clang)
```bash
# Configure with explicit build type
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build
```

#### Top-Level CMake Configuration Options
Tether provides modular build options:

| CMake Option | Default | Description |
| :--- | :---: | :--- |
| `-DTETHER_BUILD_EXAMPLES=ON` | `ON` | Build the interactive sandbox showcase application (`tether_sandbox`). |
| `-DTETHER_BUILD_TESTS=ON` | `ON` | Build the complete CTest automated validation and behavioral test suite. |
| `-DTETHER_BUILD_RUNNERS=ON` | `ON` | Build standalone workload and benchmark CLI harnesses (`tether_bench`, `tether_test`). |
| `-DTETHER_BUILD_SHARED=ON` | `ON` | Build Tether as a shared library (`DLL` / `.so`). Set `OFF` for static library. |
| `-DCHITRA_BACKEND=AUTO` | `AUTO` | Graphics backend override (`AUTO`, `D3D11`, `GLCORE`, `METAL`, `GLES3`, `WGPU`). |

### 3. Running Targets

#### Interactive Sandbox & Widget Gallery
The sandbox launches into the interactive Widget Gallery (including enabled/disabled control pairs and a right-click context menu), alongside the Lab benchmark workbench and YAML Showcase:

```bash
# Launch interactive sandbox (Widget Gallery, Lab & Showcase)
# Windows (MSVC Release or Debug)
.\build\Release\bin\tether_sandbox.exe
# Or Debug:
.\build\Debug\bin\tether_sandbox.exe

# Linux / macOS (or Ninja)
./build/Release/bin/tether_sandbox

# Launch directly into the UI Showcase view
.\build\Release\bin\tether_sandbox.exe --showcase

# Open and visualize a benchmark telemetry JSON report
.\build\Release\bin\tether_sandbox.exe --result report.json
```

> [!TIP]
> **Sandbox assets**: The sandbox loads its UI scripts and font from `assets/UI/` and `assets/Font/` beside the executable. Tether does not automatically prepend an asset directory to application paths.

#### WebAssembly & WebGPU (In-Browser Sandbox)
Tether compiles directly to WebAssembly with native support for both WebGL2 (`GLES3`) and WebGPU (`WGPU`), rendering at full device pixel ratio (High-DPI) in a responsive browser canvas with built-in touch input support for mobile devices:

```bash
# Prerequisites: Emscripten SDK (emsdk) activated and Ninja installed

# 1. Build WebGL2 Sandbox (GLES3)
emcmake cmake -B build/wasm-webgl -G Ninja -DCMAKE_BUILD_TYPE=Release -DCHITRA_BACKEND=GLES3
cmake --build build/wasm-webgl --target tether_sandbox

# 2. Build WebGPU Sandbox (WGPU)
emcmake cmake -B build/wasm-webgpu -G Ninja -DCMAKE_BUILD_TYPE=Release -DCHITRA_BACKEND=WGPU
cmake --build build/wasm-webgpu --target tether_sandbox

# 3. Test or Serve
# Quick launch using emrun (auto-opens default browser):
emrun build/wasm-webgl/tether_sandbox.html

# Or serve via local HTTP server:
python -m http.server -d build/wasm-webgl 8080
# Open http://localhost:8080/tether_sandbox.html
```

> [!NOTE]
> **Mobile Touch Support**: WebAssembly builds automatically map touch contact events to primary pointer interactions (`POINTER_DOWN`, `POINTER_MOVE`, `POINTER_UP`), while browser viewport panning and pinch-to-zoom gestures are suppressed via `touch-action: none` on the canvas element.

#### Running the Full Automated Test Suite (CTest)
Tether validates engine behavior through automated regression and behavior tests covering memory pools, generational handles, layout solvers, widget interactions, spatial hit-testing, and YAML AST parsing & rollback:

```bash
# Execute the full automated validation suite (27 tests when rendering tests are enabled)
ctest --test-dir build -C Release --output-on-failure

# Run headless correctness checks only
ctest --test-dir build -C Release -L correctness --output-on-failure
```

#### Standalone Benchmarking Harness
```bash
# Run ECS workloads (10,000 entities across 120 measured frames)
.\build\Release\test\tether_bench.exe --scenario ecs --count 10000 --frames 120

# Run layout reflow stress testing
.\build\Release\test\tether_bench.exe --scenario layout --count 5000

# Execute the complete automated benchmark matrix across scale boundaries
powershell -ExecutionPolicy Bypass -File tests/benchmarks/run_standard.ps1 -BinaryDirectory build/Release/test
```

---

## 11. Phased Roadmap & Priority Areas

Tether is actively progressing toward subsequent engineering milestones:

* **Vinyasa Layout Integration**: Integrating the standalone Vinyasa constraint engine into core Tether layout pipelines for multi-reference anchoring, flex wrapping, and scratch-arena memory reuse.
* **Ahead-of-Time (AOT) Bytecode Pipeline (`.tbc`)**: A standalone compiler tool (`tether_cooker`) producing flat binary bytecode assets for instant, sub-millisecond cold boot on microcontrollers without runtime text parsers.
* **Overflow Control System**: Container overflow policies (`WRAP`, `CLIP`, `SCROLL`, `VISIBLE`) with smooth kinetic scrolling viewports.
* **Modern UI Aesthetics**: Independent asymmetrical corner radii, soft ambient drop shadows, procedural color gradients, and glassmorphism.
* **Extended Typography**: HarfBuzz and FreeType backend integration for full Unicode bidirectional text layout (Arabic, Hebrew) and complex script ligature shaping (Devanagari, Thai).
* **Official Multi-Language Bindings**: Idiomatic zero-friction bindings for Rust, Python, WebAssembly (WASM), Lua, and Zig.

---

## 12. Contributing

Contributions are welcome. Before opening a Pull Request, please read the [Contributing Guide](CONTRIBUTING.md) for Tether's layer boundaries, engineering rules, local build/test instructions (including AddressSanitizer verification), and PR workflow.

---

## 13. License

Tether is licensed under the [Apache License 2.0](LICENSE).

### Third-Party Licenses
* **[Chitra](https://github.com/Yuvi-GD/Chitra):** Apache 2.0 / MIT
* **[libyaml](https://github.com/yaml/libyaml):** MIT License (Kirill Simonov)
