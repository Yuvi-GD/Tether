# Tether UI Engine

Tether is a high-performance, zero-allocation universal user interface engine. It serves as a micro-kernel for UI, designed to scale seamlessly from bare-metal microcontrollers to high-performance WebGPU desktop applications. 

Currently, the project is in early active development. The foundation is being laid out, and the core architecture is being built piece by piece.

## The Vision

Most modern UI frameworks carry massive overhead. They rely on heavy DOM trees, garbage collection, and bloated native OOP classes. Tether takes a different approach:

* **Zero-Allocation ECS:** The runtime engine never dynamically allocates memory or creates garbage collection churn during layout loops. It utilizes a Data-Oriented Entity Component System (ECS) with perfectly packed arrays.
* **The 4-Pipeline Deployment:** It features a JIT YAML Parser for rapid designer iteration, and an AOT Bytecode Interpreter (`.tbc` files) for absolute execution speed on microcontrollers and Apple hardware.
* **100% Pure C:** The entire engine is written in pure C. It avoids C++ inheritance overhead, ensuring flawless cross-platform compilation and extreme cache-locality.

## Project Structure

The codebase is organized into core engine files (`src/tether.c`) and specialized platform backends located in `src/backends/`. The backend directory is split into:
* **Hardware Abstraction Layer (HAL):** Implementations for windowing, OS event polling, and swapchain management.
* **Rasterizer:** Implementations for 2D vector graphics and pixel rendering.

The primary cross-platform sandbox uses `sokol_app` for the HAL and `ThorVG` for the WebGPU rasterizer.

## Build System Overview

Tether uses a multi-tiered build system by design:

* **Sokol:** Header-only library with zero build configuration.
* **ThorVG:** Built using Meson to seamlessly integrate with upstream updates.
* **wgpu-native:** Pre-built binary downloaded automatically.
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