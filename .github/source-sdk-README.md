# Tether source SDK

This archive contains the Tether library, its default Chitra renderer, and libyaml.
It does not include the sandbox, tests, benchmark runners, or a font. Applications
provide their own fonts. The repository contains examples and development tools.

## Build

Install CMake 3.20 or later, C and C++ compilers, and the graphics and platform
dependencies for your target. From the extracted directory:

```sh
cmake -S . -B build -DTETHER_BUILD_SHARED=OFF
cmake --build build --config Release
```

For a shared library, configure with `-DTETHER_BUILD_SHARED=ON` where supported.
Android, iOS, and WebAssembly builds additionally need their respective platform
toolchains. The source package does not guarantee support for every target.

To include Tether in a CMake project, use `add_subdirectory` on this directory
and link your target to `Tether::tether`. The repository has more examples and
platform build information: https://github.com/Yuvi-GD/Tether

For an Emscripten application, include `cmake/TetherWasm.cmake` and call
`tether_configure_wasm_target(your_target)` after creating the target. The SDK
includes a minimal default HTML shell; pass `SHELL <path>` to use your own.

## Shaders

The original GLSL and generated C headers are included. Normal builds do not
need the shader compiler. If you change a shader, regenerate both headers with
the matching `sokol-shdc` tool from Chitra's pinned sokol-tools dependency.

## Licenses

Tether and Chitra are licensed under Apache License 2.0; see `LICENSE` and
`third_party/chitra/LICENSE`. libyaml's license is in `third_party/libyaml/License`.
Sokol's license is in `third_party/chitra/third_party/sokol/LICENSE`. Fontstash
and stb license notices are included in their respective header files under
`third_party/chitra/third_party/`.
