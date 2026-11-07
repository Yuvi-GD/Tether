# Tether UI Engine
*Version 0.0.2 (Developer Preview)*

Tether is an open, modular, zero-allocation universal user interface engine written in pure ISO C99. Engineered around the **"LLVM of User Interfaces"** architectural paradigm, Tether decouples declarative interface authoring from underlying display hardware, establishing an open foundation for interactive graphical software that scales from sub-64 KB bare-metal microcontrollers to high-performance desktop applications, game HUDs, and WebAssembly.

---

## 1. Architectural Vision: The "PDF of Interactive User Interfaces"

Digital computing solved media portability decades ago by establishing open, vendor-neutral file specifications:
- **Audio has MP3 / FLAC**: Plays consistently on any speaker, smartphone, or dashboard, isolated from DAC hardware or audio drivers.
- **Images have PNG / JPEG**: Renders the exact same pixels across all viewers, isolated from graphics cards.
- **Documents have PDF (Portable Document Format)**: Preserves typography, spatial layout, and vector curves identically across all operating systems and printers.

**User interfaces have historically lacked this universal standard.**

For decades, graphical interfaces have remained trapped inside vendor-locked compilers (Win32, Cocoa, Qt), proprietary game engine codebases, or bloated multi-hundred-megabyte browser engines (Chromium/Electron).

Tether establishes user interfaces as a **universal, open media format**. A single declarative interface definition or compact Ahead-of-Time (AOT) bytecode asset (`.tbc`) describes the complete layout hierarchy, styling rules, typography, and interaction behavior. Any platform embedding the Tether micro-kernel opens and executes that UI asset with **100% behavioral determinism, layout consistency, and interactive fidelity**:
- An automotive touchscreen or medical monitor running an RTOS,
- A bare-metal microcontroller driving an SPI or parallel LCD,
- A modern web browser executing WebAssembly via WebGPU or Canvas,
- A high-end desktop workstation running DirectX 11, Metal, or Vulkan,
- An AAA game engine viewport in Unreal, Unity, or custom engines.

---

## 2. Core Architectural Highlights

Tether abandons the traditional Document Object Model (DOM), virtual DOM diffing, and deeply nested C++ object-oriented inheritance trees in favor of six foundational systems-level properties:

### 1. Zero-Power Idle State (0% CPU, 0 Draw Calls)
Unlike immediate-mode toolkits (such as Dear ImGui) that execute continuous frame polling loops (burning 100% CPU and GPU cycles even when the display is completely static), Tether operates on a retained-mode model governed by fine-grained reactive invalidation tags (`DIRTY_LAYOUT`, `DIRTY_VISUAL`, `DIRTY_HIERARCHY`). When no user interaction occurs and animations are at rest, Tether enters an event-driven sleep state, consuming **zero CPU cycles** and issuing **zero GPU draw calls**.

### 2. Kilobyte-Scale Data-Oriented UI-ECS
UI components (transforms, shapes, typography, styles, interaction hit-boxes) are organized into flat, type-uniform dense arrays indexed by 64-bit sparse set maps. For a typical user interface containing hundreds of active widgets, Tether's core component data consumes mere **tens of kilobytes** in memory. On bare-metal microcontrollers with software framebuffers, the entire UI system operates comfortably within **sub-64 KB SRAM** constraints.

### 3. Zero Steady-State Heap Allocations
Memory buffers for component pools grow geometrically during initial application startup into pre-sized high-water pools. Once initialized, **zero dynamic memory allocations (`malloc`, `free`) occur during frame loops, layout reflow passes, or input event dispatching**. This eliminates memory fragmentation and eradicates garbage collection micro-stutter (jank) entirely.

### 4. Separation of Layout Space from Render Space
In traditional web browsers and mobile toolkits, modifying a visual property (such as hover scale, a subtle rotation, or an opacity fade) invalidates container layout, triggering costly cascading reflow storms across parent and sibling elements. Tether mathematically decouples layout slot geometry (`Tether_SlotTransform`) from visual GPU transformations (`Tether_RenderTransform`). High-frequency animations execute purely on GPU shaders with **zero CPU layout reflow passes**.

### 5. Universal 64-Bit Generational Handles (`Tether_GUID`)
Entities are addressed exclusively by value through 64-bit scalar identifiers containing an array slot index and a generational version counter. Tether never exposes raw, unsafe memory pointers across API boundaries. Stale handles held by foreign scripting languages (Rust, Python, Lua, WebAssembly) fail safely in $O(1)$ constant time via `tether_ecs_is_valid()`, eliminating dangling pointers and use-after-free bugs.

### 6. Limitless Dual Authoring Parity
Declarative markup (YAML) and compiled code (pure C99) operate with complete parity:
- Interfaces declared in YAML and interfaces declared imperatively via the fluent `tether_ui` API instantiate the exact same ECS components.
- Tether supports live runtime UI scene refresh, allowing developers to reload and update declarative definitions in memory during development.

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
        LayoutInterface["Layout Interface & Strategy Registry"]
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
* **Branchless Analytical SDF Primitives:** Evaluates rounded rectangles, uniform corner radii, strokes, and drop shadows directly inside fragment shaders on single instanced quads without polygonal mesh tessellation or vertex buffer explosion.
* **Dynamic Glyph Atlas:** Integrates Fontstash for high-performance glyph rasterization and texture caching.
* **Hardware Scissor Clipping:** Nested clipping regions are calculated hierarchically and submitted directly to hardware scissor registers.
* **Driver-Agnostic Abstraction:** Chitra supports multiple low-level backends (DirectX 11, Apple Metal, OpenGL Core, OpenGL ES) through Sokol.

---

## 5. Documentation Hub

Comprehensive, production-grade documentation is available in the [`docs/`](docs/) directory:

> [!WARNING]
> **Developer Preview**: Tether is currently under active development (`v0.0.2-preview`). Architectural specifications, API signatures, and roadmap features reflect preview builds.

* **[Master Documentation Index](docs/index.md)**: Master navigation hub and ecosystem architecture overview.
* **[Category A: Project Presentation (10 Chapters)](docs/index.md#category-a-project-presentation)**:

* **[Category B: Technical & Developer Documentation (14 Chapters)](docs/index.md#category-b-technical-and-developer-documentation)**: Complete engineering manuals covering the ECS Kernel, memory layout, components, invalidation lifecycles, widget registries, layout engines, backend interfaces, GPU rendering, input hit-testing, declarative parsing, language bindings, API reference, benchmarks, and architectural decision records (ADRs).

---

## 6. Dependencies

Tether core requires zero dynamic external runtime libraries beyond the standard C library. The build vendors two submodule dependencies:

| Dependency | Location | Purpose |
| :--- | :--- | :--- |
| **[Chitra](https://github.com/Yuvi-GD/Chitra)** | `third_party/chitra` | Retained-mode 2D GPU rendering engine and windowing abstraction |
| **[libyaml](https://github.com/yaml/libyaml)** | `third_party/libyaml` | Fast C YAML parsing for declarative layout definitions |

---

## 7. Prerequisites

* **CMake:** Version 3.20 or newer across all platforms.
* **Windows:** Visual Studio 2022 (MSVC with "Desktop development with C++") or LLVM Clang.
* **Linux:** GCC or Clang, with development packages for X11 and OpenGL:
  * Ubuntu / Debian: `sudo apt install -y build-essential cmake libx11-dev libxi-dev libxcursor-dev libgl1-mesa-dev`
  * Fedora / RHEL: `sudo dnf install -y gcc gcc-c++ cmake libX11-devel libXi-devel libXcursor-devel mesa-libGL-devel`
  * Arch Linux: `sudo pacman -S --needed base-devel cmake libx11 libxi libxcursor mesa`
* **macOS:** Xcode Command Line Tools (`xcode-select --install`) and CMake (`brew install cmake`).

---

## 8. Building & Running

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
| `-DCHITRA_BACKEND=AUTO` | `AUTO` | Graphics backend override (`AUTO`, `D3D11`, `GLCORE`, `METAL`, `GLES3`). |

### 3. Running Targets

#### Interactive Sandbox Showcase
The sandbox is a standalone showcase illustrating declarative YAML layouts, C widget composition, runtime scene refresh, and benchmark visualization:

```bash
# Launch interactive workbench (Lab & Showcase)
# Windows (MSVC)
.\build\Release\tether_sandbox.exe
# Linux / macOS (or Ninja)
./build/tether_sandbox

# Launch directly into the UI Showcase view
.\build\Release\tether_sandbox.exe --showcase

# Open and visualize a benchmark telemetry JSON report
.\build\Release\tether_sandbox.exe --result report.json
```

#### Running the Full Automated Test Suite (CTest)
Tether validates engine behavior through automated regression and behavior tests covering memory pools, generational handles, layout solvers, spatial hit-testing, and YAML AST parsing:

```bash
# Execute the full 22-test automated validation suite
ctest --test-dir build -C Release --output-on-failure
```

#### Standalone Benchmarking Harness
```bash
# Run ECS churn workloads (10,000 entities, 1,000 iterations)
.\build\Release\tether_bench.exe --workload ecs_churn --entities 10000 --iterations 1000

# Run layout reflow stress testing
.\build\Release\tether_bench.exe --workload layout_reflow --entities 5000

# Execute the complete automated benchmark matrix across scale boundaries
powershell -ExecutionPolicy Bypass -File tests/benchmarks/run_standard.ps1
```

---

## 9. Phased Roadmap & Priority Areas

Tether is actively progressing toward subsequent developer preview milestones:

* **Vinyasa Layout Integration**: Integrating the standalone Vinyasa constraint engine into core Tether layout pipelines for multi-reference anchoring, flex wrapping, and scratch-arena memory reuse.
* **Ahead-of-Time (AOT) Bytecode Pipeline (`.tbc`)**: A standalone compiler tool (`tether_cooker`) producing flat binary bytecode assets for instant, sub-millisecond cold boot on microcontrollers without runtime text parsers.
* **Overflow Control System**: Container overflow policies (`WRAP`, `CLIP`, `SCROLL`, `VISIBLE`) with smooth kinetic scrolling viewports.
* **Modern UI Aesthetics**: Independent asymmetrical corner radii, soft ambient drop shadows, procedural color gradients, and glassmorphism.
* **Extended Typography**: HarfBuzz and FreeType backend integration for full Unicode bidirectional text layout (Arabic, Hebrew) and complex script ligature shaping (Devanagari, Thai).
* **Official Multi-Language Bindings**: Idiomatic zero-friction bindings for Rust, Python, WebAssembly (WASM), Lua, and Zig.

---

## 10. License

Tether is licensed under the [Apache License 2.0](LICENSE).

### Third-Party Licenses
* **[Chitra](https://github.com/Yuvi-GD/Chitra):** Apache 2.0 / MIT
* **[libyaml](https://github.com/yaml/libyaml):** MIT License (Kirill Simonov)
