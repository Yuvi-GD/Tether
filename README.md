# Tether UI Engine

Tether is a high-performance, zero-allocation universal user interface engine. I am building this to serve as a micro-kernel for UI, designed to scale seamlessly from bare-metal microcontrollers to high-performance WebGPU desktop applications. 

Currently, the project is in early active development. The foundation is being laid out, and the core architecture is being built piece by piece.

## The Vision

Most modern UI frameworks carry massive overhead. They rely on heavy DOM trees, garbage collection, and bloated text parsing at runtime. Tether takes a different approach:

* **Zero-Allocation Execution:** The runtime engine never allocates memory or creates garbage collection churn during layout or rendering loops. It uses fixed memory arenas.
* **Universal Tokenization:** No text parsing happens at runtime. The UI design is compiled into a highly compressed stream of hexadecimal tokens (the Asset Cooker) before it ever reaches the device.
* **Strict Decoupling:** The framework is split between a pure C micro-kernel (Layer 0) for hardware abstraction, and a stripped-down C++ layer for structural primitives. It does not force you into a specific rendering backend or operating system driver.

## Project Structure

The codebase is structured to allow developers to include only what they need, minimizing binary bloat:

* `include/` : The core Tether headers.
* `src/` : The implementation files for the engine and official backends.
* `third_party/` : Dependency headers (like Sokol, ThorVG) kept completely isolated from the core API.
* `tests/` : Sandbox environments for testing the build and rendering pipelines.

## Local Setup & Build Guide

Tether uses CMake to manage cross-platform builds. To get the sandbox environment running locally on Windows or Linux:

1. Clone the repository:
```bash
   git clone https://github.com/yourusername/Tether.git
```
2. Navigate into the directory:
   `cd Tether`
3. Create a build directory:
   `mkdir build && cd build`
4. Generate the build files:
   `cmake ..`
5. Compile the sandbox:
   `cmake --build .`

## Contributing

Since Tether is in its foundational stage, the architecture is still shifting. If you are interested in zero-bloat systems, custom bytecodes, or vector rasterization, feel free to open an issue to discuss ideas before submitting large pull requests. I am open to architectural feedback and testing on edge-case hardware.

## License

This project is licensed under the [Apache License 2.0](LICENSE). It is completely free, open-source, and open to universal adaptation and contribution.