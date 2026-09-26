-# Tether UI: Comprehensive Architecture Specification

This document serves as the absolute, comprehensive architectural blueprint for Tether, a high-performance, zero-allocation universal user interface engine. It is designed to scale seamlessly from ultra-low-power microcontrollers up to massive desktop, web, and spatial computing environments, setting a new industry standard that surpasses the Web DOM, Flutter, and traditional native frameworks.

---

## 1. Core Architectural Philosophies

Tether rejects the bloated abstraction layers of modern web DOMs and legacy native frameworks. The system is built entirely on four non-negotiable principles:

- **Zero-Allocation Execution:** The runtime engine never dynamically allocates memory via a Garbage Collector during layout or rendering loops. It achieves 100% memory stability through an Entity Component System (ECS).
- **Universal Declarative Layout:** UI is authored in a clean, human-readable format (YAML) that is parsed instantly at runtime (JIT) or compiled into ultra-dense bytecode (AOT) for embedded hardware.
- **Strict Decoupling of Logic:** The UI structure is purely visual. It contains no programming logic, `if` statements, or loops. Logic remains securely in the host language (C/C++, Python, JS), bridging to the UI via direct Data Binding.
- **Abstracted Rendering:** Tether calculates math and hit-boxes, but it delegates all pixel plotting to a swappable backend (like ThorVG) via a rigid Hardware Abstraction Layer (HAL).

---

## 2. The Multi-Tiered Runtime System

To prevent framework bloat on resource-constrained hardware, the engine is physically separated into distinct execution tiers.

### Layer 0: The Absolute Core (The Micro-Kernel)
This is the mandatory foundation layer required to boot the Tether engine. It contains zero visual components or layout rules. It operates identically to the lowest levels of Unreal Slate. It is written in pure C.
- **The Engine Loop:** Processes events, advances the frame state, and dispatches render commands.
- **The Memory ECS Manager:** Manages the Sparse Sets, Dense Arrays, and GUID tracking.
- **The Hardware Abstraction Layer (HAL):** Intercepts windowing events and talks to the graphics driver.

### Layer 1: The Primitive & Layout Layer
Visual elements and layout math are inextricably linked. This layer defines what exists on screen and how it mathematically aligns.
- **Native Primitives:** Standard modules written in strict C/C++ to guarantee raw execution speed. The core primitives include:
  - `Panel`: An invisible container used purely for layout and hierarchy.
  - `Border`: A visible container with background color, rounded corners, and strokes.
  - `Text`: Hardware-accelerated text rendering (via ThorVG/FreeType).
  - `Path`: Pure mathematical vector graphics (SVG-style lines and curves).
  - `Image`: Bitmap texture rendering.
  - `Interactable`: An invisible hit-box component that catches mouse/touch events.
- **Dynamic Primitives:** Custom, developer-defined low-level UI elements (like a custom 3D model viewer or proprietary rendering graph). These can be dynamically loaded at runtime as native shared libraries (`.dll`/`.so`), WebAssembly (WASM), or custom bytecode plugins.

---

## 3. The Memory Model: UI-ECS (Entity Component System)

Traditional UI frameworks (React, Vue, WPF) allocate massive objects scattered across the heap, causing severe memory fragmentation and requiring constant Garbage Collection. Tether solves this entirely using an **Entity Component System (ECS)**.

In Tether, a "Button" is not an object. It is simply a mathematical concept defined by a 64-bit **Globally Unique Identifier (GUID)**. Its data (Transform, Color, Text) is stripped apart and stored in strictly typed, contiguous arrays called **Components**. By using GUIDs instead of raw array indices, the Entity ID can be safely passed to external environments (like a networked server, a host Python app, or a database) without risk of desynchronization.

The 64-bit GUID is strictly constructed as `[32-bit Version | 32-bit Index]`. The index directly maps to the entity's memory arrays, while the version counter increments every time an index is recycled. This pure-math strategy strictly eliminates the "Dangling Pointer" (Use-After-Free) problem with zero memory allocation.

### 3.1 Sparse Sets and Packed Arrays
To prevent memory holes (fragmentation), Tether isolates component data using the **Sparse Set** pattern:
1. **The Dense Array:** A perfectly packed block of memory containing only the data (e.g., an array of `Transform` structs).
2. **The Sparse Map:** A lookup array mapping an `Entity ID` to its current index in the Dense Array.

If Entity `42` has a Transform and Entity `44` has a Transform, but Entity `43` does not, the Dense Array only contains two entries. Entity `43` consumes zero bytes of memory. This achieves absolute maximum CPU Cache-Locality.

### 3.2 The Swap-and-Pop Deletion Mechanic
When UI elements are dynamically destroyed, traditional allocators leave holes. In Tether's UI-ECS:
1. When an Entity is destroyed, the Engine targets its data in the Dense Array.
2. It takes the **very last item** in the array and overwrites the deleted item's memory slot.
3. The Dense Array is shrunk by 1.
4. The Sparse Map is updated to reflect the new index of the swapped item.
This executes in `O(1)` constant time. The memory array remains 100% perfectly packed, guaranteeing zero fragmentation.

### 3.3 Arena Allocation and Memory Security
Tether utilizes a High-Water Mark memory strategy. When capacity is exceeded, the Dense Array automatically scales up exponentially (via `realloc`) to support massive UIs without crashing. 
Crucially, any reserved "empty space" is completely protected by the Sparse Map. Attempting to query an inactive entity simply returns `NULL`, making memory leaks, use-after-free bugs, and unauthorized pointer exploits mathematically impossible.

### 3.4 Data-Oriented Small String Optimization (SSO)
Tether completely avoids Garbage Collected string memory via a specialized Small String Optimization implementation tailored for ECS:
- **Style Separation**: Visual text properties (size, font, color, and manual `wrap_width` limits) are decoupled into a purely mathematical `Tether_Text` component.
- **Fixed-Size Inline Arrays**: The physical character strings are baked directly into fixed-size component structs (`Text_Word` at 32 bytes, `Text_Label` at 128 bytes, and `Text_Paragraph` at 512 bytes). This guarantees that the layout engine and rasterizer load the Component Data and the String Data simultaneously in a single L1 CPU cache fetch, eliminating all pointer-chasing and cache misses.
- **Dynamic Capacity Fallback**: For massive bodies of dynamically changing text (like an integrated code editor), Tether falls back to a `Text_Dynamic` component. This is the exclusive text component that interacts with the heap, implementing geometric capacity growth to prevent constant OS-level `realloc` calls during high-speed typing.

### 3.5 O(1) Doubly-Linked Hierarchy & Cascading Deletion
Tether completely isolates parent/child layout trees from memory pointers. The UI tree is flattened using a strict Doubly-Linked List architecture inside a standard `Tether_Hierarchy` component (storing `parent`, `first_child`, `last_child`, `prev_sibling`, and `next_sibling` as pure GUIDs).
- **O(1) Appending & Detaching:** Because every sibling tracks its neighbors, and every parent tracks its `last_child`, attaching an entity or instantly patching a hole during removal executes in `O(1)` constant time, requiring zero pointer looping.
- **Cascading Deletion:** When a parent is destroyed, the engine recursively cascades the destroy command through all children via value-copy isolation. The engine natively avoids wasteful tree detaching during full teardowns, ensuring extreme performance.

### 3.6 Opaque Dynamic Component Registration
Tether acts as an infinitely extensible substrate. It defines a configurable capacity limit (`TETHER_MAX_COMPONENT_TYPES`, defaulting to 128). 
- **Internal Reservation:** IDs 0 through 31 are strictly reserved for native engine primitives (Hierarchy, Color, Transform, etc.), allowing instantaneous index lookups without string hashing.
- **Third-Party Extensions:** Custom developers can dynamically request Opaque Component IDs starting at 32+. The ECS manages these third-party dense arrays blindly, applying the exact same blazing-fast caching and memory packing algorithms automatically.

---

## 4. The Deployment Pipeline: 4 Ways to Run Tether

Tether treats UI just like a compiled programming language, utilizing 4 distinct tools depending on the hardware target:

1. **The Parser (Development / Web / Desktop):** A lightweight C text parser that reads human-readable `.yaml` strings and instantly constructs the ECS memory arrays at runtime. Perfect for rapid designer iteration.
2. **The Asset Cooker (AOT Compiler):** A standalone developer tool that compiles `.yaml` text into highly compressed hexadecimal bytecode (`.tbc`). Used ahead-of-time before shipping a game or embedded app.
3. **The Interpreter (iOS / Mac / Microcontrollers):** A hyper-fast C runtime that reads `.tbc` bytecode directly into memory. It bypasses string parsing entirely and complies with strict Apple security constraints (no `PROT_EXEC` required).
4. **The JIT Compiler (Heavy Dynamic Logic):** If a developer writes complex custom logic (Dynamic Primitives), the JIT compiles that logic into native machine code at runtime for maximum speed (supported on Windows/Linux/Android, but blocked on Apple devices where the Interpreter takes over).

### 4.1 YAML & Code Interoperability (Dual-UI Generation)
Because Tether is an ECS, creating UI from a YAML file or from C Code executes the exact same underlying logic. The ECS memory serves as the single source of truth. Developers can write UI purely in YAML, dynamically generate hundreds of items purely via the C API (`tether_ecs_create_entity()`), or seamlessly mix both—such as loading a static YAML layout and injecting dynamic C-driven children into it at runtime.

---

## 5. Separation of Layout and Logic (Event-Driven Data Binding)

Tether strictly forbids programming logic (loops, `if/else` statements, event graphs) from residing in the UI script. The YAML script defines the layout tree and visual states. The logic remains entirely in the Host Language (C, C++, Python, TS).

**The Event-Driven Bridge (Push, not Pull):**
To ensure absolute zero-overhead, Tether *does not poll* memory every frame.
1. The Programmer allocates a variable in the Host Language (e.g., `health = 100`).
2. The Designer binds the UI to this ID in YAML (`width: bind(Health)`).
3. When the Programmer modifies the variable, they fire a signal: `Tether_MarkDirty("Health", new_value)`.
4. Tether instantly updates the exact ECS memory slot and redraws only that bounding box. The C-Engine sleeps entirely until the host explicitly pushes an update.

---

## 6. Graphics, Icons, and 3D Pipeline Integration

### 6.1 Vector Path Primitives (ThorVG)
To completely avoid the parsing overhead of external SVG files or the memory bloat of PNG bitmaps, all icons and vector assets are stored directly inside the UI script as a **Path Primitive**. Vector shapes are written as raw math instructions compressed into tokens and drawn natively by the hardware rasterizer (ThorVG). ThorVG is completely blind to UI state or hit-boxes; it simply receives raw mathematical coordinates from the Tether kernel and draws anti-aliased pixels.

### 6.2 The Pure 2D Projection Strategy (Spatial Computing)
Tether remains a dedicated, hyper-focused 2D layout engine. To support 3D scenes, editor gizmos, or VR environments, it uses two distinct methods:
- **Compositing Viewports:** Tether renders the 2D frame interface cleanly but leaves an explicit mathematical clipping window (a "hole") in the layout coordinates. It hands off this screen real estate to a dedicated 3D thread (WebGPU, Vulkan, Unreal Engine) to draw 3D assets directly underneath the UI layer.
- **Spatial Texture Projection:** For world-space UIs (in-game computer screens or VR floating panels), Tether renders the entire interface layout onto an isolated, off-screen 2D texture buffer. The external 3D engine then maps this flat texture onto any 3D polygonal geometry in virtual space.

### 6.3 Damage-Tracked Animations
Animations bypass CPU layout passes entirely via **Direct-to-Shader Property Tweens**. Layout dimensions remain completely static on the CPU, while visual animations (offsets, alpha fades) update as uniform variables directly inside the hardware shader pipeline. The engine utilizes pixel region damage-tracking, recalculating only the exact bounding boxes that experience visual updates.

---

## 7. Hardware Abstraction Layer (HAL) & Input

To preserve absolute isolation, Tether splits drawing mechanics and platform management into decoupled abstract layers.

### 7.1 Link-Time Backend Separation
Tether abstracts all platform dependencies behind an immutable, zero-cost header contract (`tether_hal.h` and `tether_raster.h`).
- **The Primary Path:** Links `sokol_hal.c` and `thorvg_raster.c`, providing an optimized, cross-platform layer handling windowing and multi-backend graphics translation (WebGPU/Metal/DX12).
- **The Bare-Metal Path:** For microcontrollers lacking an OS, the driver directly addresses hardware registers to flush the Tether display list to a direct CPU frame buffer.

### 7.2 Input Normalization
Tether does not distinguish between a desktop mouse and a mobile touchscreen. All hardware input is intercepted by the platform driver and translated into a unified `Tether_Pointer_Event`. 
To handle continuous inputs (like rapid hovering or dragging) without wasting CPU cycles, Tether implements **State Caching (Event Capture)**, bypassing hit-testing entirely for active interactions. For raw hit-testing, Tether utilizes **Hierarchical Culling**—quickly skipping vast branches of the UI tree if a parent's bounding box is not intersected, achieving blazing-fast `O(log N)` spatial queries.

---

## 8. Application Configuration (The Host Contract)

Tether is designed as an embedded kernel and will never force aggressive behaviors onto the host application. The Engine's behavior is entirely dictated by a `Tether_App_Config` structure injected at startup.
Through this configuration, the host application maintains absolute control over:
- **Memory Cleanup:** Whether the engine retains memory (High-Water Mark) or aggressively shrinks allocated capacity (e.g., `tether_ecs_shrink_to_fit`).
- **Framerate & Rendering:** Defining explicit target FPS limits or selecting between GPU backends (WebGPU/Metal) and fallback CPU software renderers.
- **Input Strategies:** Tuning hit-testing algorithms and event dispatching.

---

## 9. Dual-Language Native Compilation Model

Tether utilizes a precise language split that balances the cross-platform portability of pure C with the semantic structure required for complex visual trees.

- **The Micro-Kernel (Layer 0):** Written in **Pure C** for absolute predictability, minimal machine binary footprints, and flawless portability across hardware architectures.
- **The Primitive Layers (Layers 1-3):** Written in a **Strict Minimalist Subset of C++**.
- **Zero Runtime Bloat:** Exceptions (`-fno-exceptions`) and Run-Time Type Information (`-fno-rtti`) are completely disabled. No Standard Template Library (STL) is used. The C++ compiler produces the exact same stripped-down, ultra-lean machine assembly instructions as the pure C compiler.
- **The ABI Link Layer:** The layers communicate seamlessly using standard `extern "C"` declarations, allowing the pure C kernel to securely call structured layout functions of the primitive layer without runtime penalties.
