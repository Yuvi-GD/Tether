# Technical Specification: Tether UI Engine Architecture

This document serves as the architectural blueprint for Tether, a high-performance, ultra-lightweight, zero-allocation universal user interface engine. It is designed to scale seamlessly from ultra-low-power microcontrollers up to massive desktop, web, and spatial computing environments.

## 1. Core Architectural Philosophies

Tether rejects the bloated abstraction layers of modern web DOMs and legacy native frameworks. The system is built entirely on three non-negotiable principles:

- **Zero-Allocation Execution:** The runtime engine never dynamically allocates memory or creates garbage collection churn during layout or rendering loops.
- **Pure C Portability:** The entire engine is written in pure C. It avoids C++ inheritance overhead, ensuring flawless cross-platform compilation and extreme cache-locality.
- **Decoupled Architecture:** The UI structure is purely declarative and completely separated from application logic. Logic remains in the host language (C, Python, JS), making the rendering pipeline safe and un-crashable.

## 2. The Multi-Tiered Runtime System

To prevent framework bloat on resource-constrained hardware, the engine is physically separated into distinct execution tiers.

### Layer 0: The Absolute Core (The Micro-Kernel)
This is the mandatory foundation layer required to boot the Tether engine. It contains zero visual components or layout rules.
- **The Memory ECS Manager:** Manages the Sparse Sets and Dense Arrays for zero-fragmentation memory.
- **Hardware Abstraction Layer (HAL):** The minimal layer that talks directly to the physical display buffer and operating system window.

### Layer 1: Primitive & Layout Registries
Visual components are treated as pure data arrays sitting on top of the Absolute Core.
```text
+-------------------------------------------------------------+
|                     DYNAMIC PRIMITIVES                      |
|       (Shared Libraries .dll/.so or Custom Bytecode)        |
+-------------------------------------------------------------+
|                      NATIVE PRIMITIVES                      |
|        (Standard Data Arrays: Transform, Text, Path)        |
+-------------------------------------------------------------+
|               LAYER 0: THE ABSOLUTE CORE                   |
|        (ECS Memory Manager | Hardware Abstraction HAL)      |
+-------------------------------------------------------------+
```
- **Native Primitives:** Standard modules that operate via pure math functions over the ECS arrays.
- **Dynamic Primitives:** Custom, developer-defined UI elements. These can be native shared libraries (`.dll`/`.so`) or, for strict environments, executed via interpreters. (Note: WASM is supported as an extension, but Tether is not fundamentally limited to it).

## 3. The 4-Pipeline Deployment System

Tether replaces rigid text files and heavy compiled binaries with a hyper-flexible, 4-stage pipeline approach, allowing developers to choose the perfect format for their target hardware:

1. **The Parser (JIT Iteration):** A lightweight C text parser that reads human-readable `.yaml` strings and instantly constructs the ECS memory arrays at runtime. Used for Designer tools (Figma-style) and rapid desktop development.
2. **The Asset Cooker (AOT Compilation):** A standalone compiler that converts `.yaml` text into highly compressed hexadecimal bytecode (`.tbc`).
3. **The Interpreter (Secure Runtime):** A hyper-fast C runtime that reads `.tbc` bytecode directly into memory. It bypasses string parsing entirely and complies with strict security constraints (like iOS) where dynamic JIT memory is forbidden.
4. **The JIT Compiler (Dynamic Logic):** Used to compile heavy custom UI logic into native machine code at runtime for maximum speed on unrestrained desktop/Android OS targets.

## 4. Memory Strategy: UI-ECS (Entity Component System)

Tether completely bypasses heap fragmentation and the DOM model by implementing a Data-Oriented Entity Component System.

- **Sparse Sets and Dense Arrays:** Instead of allocating objects, Tether strips UI elements into raw data arrays (e.g., an array of only `Transforms`, an array of only `Colors`). This achieves absolute maximum CPU Cache-Locality.
- **The Swap-and-Pop Mechanic:** When a UI element is deleted, Tether does not leave an empty memory hole. It takes the very last element in the array and swaps it into the deleted slot. This ensures the memory block remains 100% perfectly packed with zero fragmentation.
- **Atomic Data Patching:** When UI state modifications occur, the layout is never re-parsed. A localized signal from the Host App targets the exact memory offset of the changing property in the ECS, modifying it instantly in-place.

## 5. Graphics, Icons, and 3D Pipeline Integration

Graphics rendering in Tether is performance-optimized to minimize CPU and GPU tick cycles.

### Vector Path Primitives
To completely avoid the parsing overhead of external SVG files or the memory bloat of PNG bitmaps, all icons and vector assets are stored directly inside the UI scripts as a **Path Primitive**. Vector shapes are written as raw math instructions (`MoveTo`, `LineTo`, `CurveTo`, `Fill`) and drawn natively by the hardware rasterizer.

### The Pure 2D Projection Strategy
Tether remains a dedicated, hyper-focused 2D layout engine. To support 3D scenes, editor gizmos, or spatial environments, it uses two distinct methods:
- **Compositing Viewports:** Tether renders the 2D frame interface cleanly but leaves an explicit mathematical clipping window. It hands off this screen real estate to the dedicated 3D thread (such as WebGPU or Vulkan) to draw 3D assets directly underneath the UI layer.
- **Spatial Texture Projection:** For world-space UIs (VR panels), Tether renders the entire interface layout onto an isolated, off-screen 2D texture buffer. The external 3D engine then maps this flat texture onto 3D geometry in virtual space.

### The Vector Rasterization Engine (ThorVG)
Tether strictly separates UI logic from pixel rasterization. It hands a flat list of drawing commands to an isolated rasterizer.
* **The Default Engine (ThorVG):** Tether utilizes ThorVG as its primary vector rasterizer. It parses Tether's vector paths and renders them natively via CPU, WebGL, or WebGPU.
* **Strict Division of Labor:** ThorVG is completely blind to UI state or hit-boxes. It simply receives raw mathematical coordinates from the Tether ECS, draws the pixels, and exits.

## 6. Hardware Abstraction Layer (HAL) & Pluggable Drivers

To preserve absolute isolation, Tether splits drawing mechanics and platform management into decoupled abstract layers.

### Link-Time Backend Separation
Tether abstracts all platform dependencies behind an immutable, zero-cost header contract (`tether_hal.h` and `tether_raster.h`). Tether contains no platform-specific code natively natively.
* **The Primary Cross-Platform Path:** Links `sokol_hal.c` and `thorvg_raster.c`. Provides a highly optimized layer handling windowing and multi-backend graphics translation.
* **The Bare-Metal Path:** Bypasses all graphics APIs entirely. For microcontrollers lacking an OS, the driver directly addresses hardware registers to flush the Tether display list.

### Input Normalization
All hardware input is intercepted by the platform driver and immediately translated into a unified `Tether_Pointer_Event`.
* **Mathematical Hit-Testing:** When an event is fired, Tether performs a spatial integer query against the bounds of the active elements in the ECS memory arena. It checks intersections from the highest Z-Index downward, triggering the Event ID in constant time `O(1)` without relying on a slow DOM tree.
