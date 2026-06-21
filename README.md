# Tether UI Engine

Tether is a high-performance, zero-allocation universal user interface engine. I am building this to serve as a micro-kernel for UI, designed to scale seamlessly from bare-metal microcontrollers to high-performance WebGPU desktop applications. 

Currently, the project is in early active development. The foundation is being laid out, and the core architecture is being built piece by piece.

## The Vision

Most modern UI frameworks carry massive overhead. They rely on heavy DOM trees, garbage collection, and bloated text parsing at runtime. Tether takes a different approach:

* **Zero-Allocation Execution:** The runtime engine never allocates memory or creates garbage collection churn during layout or rendering loops. It uses fixed memory arenas.
* **Universal Tokenization:** No text parsing happens at runtime. The UI design is compiled into a highly compressed stream of hexadecimal tokens (the Asset Cooker) before it ever reaches the device.
* **Strict Decoupling:** The framework is split between a pure C micro-kernel (Layer 0) for hardware abstraction, and a stripped-down C++ layer for structural primitives. It does not force you into a specific rendering backend or operating system driver.

## Project Structure

```
Tether/
├── include/              # Core Tether public headers
├── src/
│   └── backends/         # Platform backend implementations
├── tests/
│   └── sandbox_main.c    # Interactive sandbox for development
├── third_party/
│   ├── sokol/            # git submodule: cross-platform windowing & input
│   └── thorvg/           # git submodule: WebGPU vector graphics renderer
│       └── builddir/     # Meson build output (generated locally, not committed)
├── cmake/                # CMake helper scripts
│   └── wgpu_native.pc.in # pkg-config template for wgpu-native
└── CMakeLists.txt        # Master build: downloads wgpu-native & builds ThorVG
```

> **Note on `third_party/webgpu/`:** This directory is created automatically by CMake when you configure the project. It contains the pre-built wgpu-native binary for your OS and is listed in `.gitignore` do not commit it.

## Build System Overview

Tether uses a **two-build-system approach** by design:

| Library | Build System | Reason |
|---|---|---|
| Sokol | Header-only (no build) | Single-file headers, zero configuration |
| ThorVG | **Meson** | ThorVG's native build system, kept unchanged for easy version updates |
| wgpu-native | Pre-built binary | No Rust toolchain required for contributors |
| Tether itself | **CMake** | Cross-platform, IDE-friendly, widely supported |

CMake orchestrates everything: it downloads wgpu-native, then invokes Meson to build ThorVG, then builds Tether all from one `cmake -B build` command.

## Prerequisites

Install these tools before building:

| Tool | Minimum Version | Install |
|---|---|---|
| CMake | 3.20 | [cmake.org](https://cmake.org/download/) or via VS 2022 installer |
| Python | 3.8+ | [python.org](https://www.python.org/) |
| Meson | 0.63 | `pip install meson` |
| Ninja | any | `pip install ninja` |
| C/C++ compiler | any modern | See platform notes below |

### Windows
- **Visual Studio 2022** (Community or higher) with the **"Desktop development with C++"** workload
- Run all commands from a **"x64 Native Tools Command Prompt for VS 2022"** (or add VS cmake to your PATH: `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`)
- Or install CMake standalone from cmake.org and add it to PATH

### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake python3 python3-pip pkg-config \
                 libx11-dev libxi-dev libxcursor-dev
pip install meson ninja
```

### macOS
```bash
xcode-select --install          # Xcode command line tools
brew install cmake python meson ninja
```

## Building (All Platforms)

**1. Clone with submodules:**
```bash
git clone --recurse-submodules https://github.com/Yuvi-GD/Tether.git
cd Tether
```

> If you already cloned without submodules:
> ```bash
> git submodule update --init --recursive
> ```

**2. Configure this downloads wgpu-native and builds ThorVG automatically:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

**3. Build the sandbox:**
```bash
cmake --build build
```

**4. Run:**
```bash
# Windows
build\Debug\tether_sandbox.exe

# Linux / macOS
./build/tether_sandbox
```

That's it. No separate setup scripts, no manual steps.

## Rebuilding ThorVG

ThorVG is only built once during the first `cmake -B build`. If you update the `third_party/thorvg` submodule to a newer version and want to rebuild:

```bash
# Delete the Meson build output to trigger a rebuild on next cmake configure
rm -rf third_party/thorvg/builddir

# Re-run cmake configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

## Updating wgpu-native

The version is pinned in the top of `CMakeLists.txt`:

```cmake
set(WGPU_VERSION "v29.0.0.0")
```

To upgrade: change that one line, delete `third_party/webgpu/`, and re-run `cmake -B build`. CMake will download the new version automatically for all platforms.

## Contributing

Since Tether is in its foundational stage, the architecture is still shifting. If you are interested in zero-bloat systems, custom bytecodes, or vector rasterization, feel free to open an issue to discuss ideas before submitting large pull requests. I am open to architectural feedback and testing on edge-case hardware.

## License

This project is licensed under the [Apache License 2.0](LICENSE). It is completely free, open-source, and open to universal adaptation and contribution.