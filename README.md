# Tether UI Engine

Tether is a lightweight, zero-allocation universal user interface engine written in pure C99. Designed from a systems-programming perspective, it operates as a minimalist UI micro-kernel that scales from bare-metal microcontrollers to high-performance desktop applications.

---

## Core Architecture & Pillars

* **Zero-Allocation Core Loop:** The runtime engine never dynamically allocates memory or creates garbage collection churn during steady-state layout or rendering loops. All operations execute on pre-allocated, contiguous arrays.
* **Data-Oriented ECS:** UI components (transforms, shapes, text, styles, hit-boxes) are organized into flat tables indexed by dense entity IDs for optimal cache locality.
* **The 4-Pipeline Deployment:** Supports a JIT YAML Parser for instant visual iteration during development, and an AOT Bytecode Interpreter (`.tbc` files) for constrained embedded targets requiring zero runtime parsing overhead.
* **Pure C99 Implementation:** 100% C99 with strict ABI stability, zero OOP overhead, and predictable memory layout.
* **Reactive Dirty-Flag Layout:** Hierarchical bounding boxes and intrinsic dimensions are re-evaluated only when nodes are marked dirty.

---

## Rendering Pipeline: Chitra

Tether decouples UI orchestration from graphics hardware via an abstract Render Hardware Interface (RHI). Core 2D rendering is driven by **[Chitra](https://github.com/Yuvi-GD/Chitra)**, a modular retained-mode 2D graphics subsystem:

* **Retained Command Table:** UI primitives are tracked in contiguous 16-byte command slots with 64-bit visual sort keys.
* **LSD Radix Sort Batching:** Coalesces draw calls by pipeline state, scissor clip regions, and texture bindings.
* **Branchless Analytical SDF Primitives:** Evaluates rounded rectangles, borders, and variable corner radii without path tessellation, mesh buffers, or GPU discard pipeline stalls.
* **Hardware Scissor Clipping:** Nested clipping regions are calculated hierarchically and submitted directly to hardware scissor registers.
* **Driver-Agnostic Architecture:** Chitra is designed to support multiple hardware abstraction drivers (including Sokol for Direct3D 11 / Metal / OpenGL, SDL, and bare-metal rasterizers).

### Planned Extensions
* **Renderer Backends:** Future bridges for SDL, Raylib, and Skia.
* **Input Drivers:** Future support for Gamepad, Multi-Touch gestures, and Stylus input.

---

## Dependencies

Tether strictly requires only two vendored git submodules:

| Dependency | Location | Purpose |
| :--- | :--- | :--- |
| **[Chitra](https://github.com/Yuvi-GD/Chitra)** | `third_party/chitra` | Retained-mode 2D rendering engine and windowing abstraction |
| **[libyaml](https://github.com/yaml/libyaml)** | `third_party/libyaml` | Fast C YAML parsing for declarative layout definitions |

---

## Prerequisites

* **CMake:** Version 3.20 or newer across all platforms.
* **Windows:** Visual Studio 2022 (MSVC with "Desktop development with C++") or LLVM Clang.
* **Linux:** GCC or Clang, with development packages for X11 and OpenGL:
  * Ubuntu / Debian: `sudo apt install -y build-essential cmake libx11-dev libxi-dev libxcursor-dev libgl1-mesa-dev`
  * Fedora / RHEL: `sudo dnf install -y gcc gcc-c++ cmake libX11-devel libXi-devel libXcursor-devel mesa-libGL-devel`
  * Arch Linux: `sudo pacman -S --needed base-devel cmake libx11 libxi libxcursor mesa`
* **macOS:** Xcode Command Line Tools (`xcode-select --install`) and CMake (`brew install cmake`).

---

## Building & Running

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
```bash
# Configure build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build all targets (engine library, sandbox, unit tests)
cmake --build build --config Release
```

#### Graphics Backend Selection (Optional)
Override the default backend using `-DCHITRA_BACKEND`:
* `-DCHITRA_BACKEND=AUTO` (default: D3D11 on Windows, Metal on macOS, GLCore on Linux)
* `-DCHITRA_BACKEND=D3D11` (Direct3D 11)
* `-DCHITRA_BACKEND=GLCORE` (OpenGL Core 4.1)
* `-DCHITRA_BACKEND=METAL` (Apple Metal)
* `-DCHITRA_BACKEND=GLES3` (OpenGL ES 3.0)

### 3. Run Targets

#### Interactive Sandbox
```bash
# Windows
.\build\Release\tether_sandbox.exe
# (or for Debug: .\build\Debug\tether_sandbox.exe)

# Linux / macOS
./build/tether_sandbox
```

#### ECS Unit Tests
```bash
# Windows
.\build\Release\tether_ecs_test.exe
# (or for Debug: .\build\Debug\tether_ecs_test.exe)

# Linux / macOS
./build/tether_ecs_test
```

---

## Contributing

Contributions that adhere to the engine architecture and zero-allocation principles are welcome.

### Guidelines
* **Follow Architecture:** Adhere strictly to pure C99 and zero dynamic allocations (`malloc`/`free`) on frame or layout execution paths.
* **Discuss Major Changes:** Open an issue to discuss design tradeoffs before submitting large architectural changes or new component types.
* **Commit Quality:** Write concise, technically precise commit messages describing exact systems changes. Verify that unit tests (`tether_ecs_test`) pass before submitting pull requests.

### Priority Areas
* Complex text shaping and bidirectional layout integration.
* AOT Bytecode compiler and virtual machine execution engine (`.tbc`).
* Advanced layout constraints (flex-wrap, grid alignment).
* Additional hardware abstraction backends for Chitra (SDL, Skia).
* Input device drivers for touch gestures and gamepad navigation.

---

## License

Tether is licensed under the [Apache License 2.0](LICENSE).

### Third-Party Licenses
* **[Chitra](https://github.com/Yuvi-GD/Chitra):** Apache 2.0 / MIT
* **[libyaml](https://github.com/yaml/libyaml):** MIT License (Kirill Simonov)