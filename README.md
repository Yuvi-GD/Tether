# Tether UI Engine

Tether is a high-performance, zero-allocation universal user interface engine. It serves as a micro-kernel for UI, designed to scale seamlessly from bare-metal microcontrollers to high-performance WebGPU desktop applications. 

## The Vision & The Problem We Solve

Modern UI frameworks carry massive overhead. They rely on heavy DOM trees, garbage collection, bloated native OOP classes, and expensive styling recalculations. Tether takes a fundamentally different approach to rendering user interfaces:

* **Zero-Allocation ECS:** The runtime engine never dynamically allocates memory or creates garbage collection churn during layout or rendering loops. It utilizes a Data-Oriented Entity Component System (ECS) with perfectly packed arrays.
* **The 4-Pipeline Deployment:** Features a JIT YAML Parser for rapid designer iteration, and an AOT Bytecode Interpreter (`.tbc` files) for absolute execution speed on constrained devices.
* **100% Pure C:** The entire engine is written in pure C. It avoids C++ inheritance overhead, ensuring flawless cross-platform compilation, ABI stability, and extreme cache-locality.

Currently, the project is in **early active development**. We have established the core ECS registry, a flexible WebGPU/ThorVG backend, and a reactive dirty-flag layout solver. We are actively optimizing the engine to guarantee absolute minimum resource footprint.

## Documentation & Architecture

For a deep dive into Tether's design principles, subsystem lifecycle, and memory management, please explore the `docs/` directory.
* See [docs/Tether_Final_Architecture.md](docs/Tether_Final_Architecture.md) for the core blueprint of the engine.

## ⚠️ Current Architecture Warning: ThorVG VRAM Usage

While Tether's CPU architecture is designed for zero-bloat, **our current WebGPU rendering backend relies heavily on ThorVG**, which introduces massive VRAM footprint issues at high resolutions.

ThorVG is a generalized vector graphics library. When used as the primary UI renderer (e.g., rendering rounded rectangles and borders), its experimental WebGPU backend tessellates every path and allocates massive internal staging and multisample buffers. It consumes roughly **~350 bytes of RAM/VRAM per pixel** of the target canvas. This means running the UI natively at 4K resolution can consume over 3GB of RAM!

**The Roadmap Fix:** We are actively pivoting the architecture to beat modern browsers. In future updates, basic UI primitives (Rectangles, Borders, Text) will bypass ThorVG entirely and be rendered natively via ultra-fast, zero-tessellation `sokol_gfx` quad shaders. ThorVG will be demoted to an "SVG Rasterizer," used only to draw complex vector icons into localized offscreen textures. Until this is implemented, the sandbox aggressively caps the offscreen render resolution (e.g., to 720p) to maintain a low memory footprint.
-  And many more Render support to comes in future like (Raylib, Skia, SDL, etc..)
-  And many more Input support to comes in future like (Touch, Controller, Gamepad, etc..)

## Project Structure

The codebase is organized into core engine files (`src/tether.c`) and specialized platform backends located in `src/backends/`. The backend directory is split into:
* **Hardware Abstraction Layer (HAL):** Implementations for windowing, OS event polling, and swapchain management.
* **Rasterizer:** Implementations for 2D vector graphics and pixel rendering.

## Acknowledgments & Thanks

Tether stands on the shoulders of incredible open-source libraries. A massive thank you to the creators of:
* **[Sokol](https://github.com/floooh/sokol):** For providing the flawless, header-only Hardware Abstraction Layer (HAL) for WebGPU and OS Windowing.
* **[ThorVG](https://github.com/thorvg/thorvg):** For the powerful vector graphics rendering backend.
* **[wgpu-native](https://github.com/gfx-rs/wgpu-native):** For the reliable WebGPU implementations across native platforms.
* **[libyaml](https://github.com/yaml/libyaml):** For parsing our declarative UI.
* **[stb_ds](https://github.com/nothings/stb):** For the robust, single-file C hash maps and dynamic arrays used throughout our registry.

## Build System Overview

Tether uses a multi-tiered build system by design:
* **Sokol:** Header-only library with zero build configuration.
* **ThorVG:** Built using Meson to seamlessly integrate with upstream updates.
* **wgpu-native:** Pre-built binary downloaded automatically.
* **libyaml:** Included as a git submodule to handle runtime YAML parsing.
* **Tether:** Built using CMake for cross-platform IDE support.

CMake orchestrates the entire process: it downloads wgpu-native, invokes Meson to build ThorVG, and builds Tether from a single configuration command.

## Prerequisites

Install these tools before building:

* **CMake:** Version 3.20 or newer.
* **Python:** Version 3.8 or newer.
* **Meson:** Install via `pip install meson` (minimum version 0.63).
* **Ninja:** Install via `pip install ninja`.
* **C/C++ compiler:** Visual Studio 2022 (Windows), GCC/Clang (Linux/macOS).

### Windows Setup
Install Visual Studio 2022 Community with the "Desktop development with C++" workload. Run all build commands from the "x64 Native Tools Command Prompt for VS 2022".

### Linux Setup (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake python3 python3-pip pkg-config libx11-dev libxi-dev libxcursor-dev
pip install meson ninja
```

### macOS Setup
```bash
xcode-select --install
brew install cmake python meson ninja
```

## Building

**1. Clone with submodules:**
```bash
git clone --recurse-submodules https://github.com/Yuvi-GD/Tether.git
cd Tether
```

**2. Configure the project (using CMake Presets):**
```bash
# Windows
cmake --preset windows-debug

# Linux
cmake --preset linux-debug

# macOS
cmake --preset macos-debug
```
Note: This step automatically downloads wgpu-native and builds ThorVG.

**3. Build the sandbox:**
```bash
cmake --build --preset windows-debug # (or your respective preset)
```

**4. Run the sandbox:**
```bash
# Windows
build\Debug\tether_sandbox.exe

# Linux and macOS
./build/tether_sandbox
```

## Maintenance

### Rebuilding ThorVG
ThorVG is built once during the initial CMake configuration. To force a rebuild after updating the submodule, delete the build directory:
```bash
rm -rf third_party/thorvg/builddir
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

### Updating wgpu-native
The version is pinned at the top of `CMakeLists.txt`. To upgrade, modify `WGPU_VERSION`, delete the `third_party/webgpu/` folder, and re-run CMake configuration.

## Contributing

The architecture is currently in a foundational stage. If you are interested in zero-bloat systems, custom bytecodes, or vector rasterization, please open an issue to discuss ideas before submitting large pull requests.

## License

This project is licensed under the [Apache License 2.0](LICENSE). It is free, open-source, and open to universal adaptation.