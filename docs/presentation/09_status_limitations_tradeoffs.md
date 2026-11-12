# Chapter 09: Current Status, Limitations, and Trade-offs

[<- Previous: Chapter 08 - Demonstrations and Verified Performance](08_demonstrations_and_benchmarks.md) | [Documentation Master Index](../index.md) | [Next: Chapter 10 - Future Roadmap and Community Participation ->](10_roadmap_and_community.md)

---

## 1. Executive Summary

True engineering maturity is demonstrated not by claiming universal perfection, but by transparently documenting architectural limitations, active constraints, and deliberate design trade-offs. No user interface framework is optimal for all computing problems:
- A framework optimized for rapid web document rendering makes severe compromises regarding memory consumption and battery life.
- A framework optimized for quick game engine debug menus makes severe compromises regarding layout capabilities and idle power draw.

Tether is engineered for **cache efficiency, predictable data ownership, and cross-platform hardware decoupling**. Achieving these properties required intentional systems-level trade-offs: prioritizing single-threaded dense-array locality over mutex-locked objects, retained invalidation flags over immediate re-submission, and an abstract `Tether_Backend` contract over platform-specific UI code.

This chapter provides an honest, comprehensive evaluation of Tether's current implementation status, its deliberate engineering trade-offs, its active technical limitations, and a practical framework for deploying Tether across diverse application domains.

---

## 2. Current Implementation Status (`v0.0.3-preview`)

Tether's architecture is categorized into subsystems implemented in the `v0.0.3-preview` codebase and active roadmap milestones:

### 1. Implemented Subsystems (`v0.0.3-preview`)
- **Data-Oriented ECS Micro-Kernel**:
  - Implemented in ISO C99 (CI validated on Windows with MSVC and Clang ASan).
  - Packed contiguous dense component arrays (`realloc` geometric growth) and 64-bit sparse set indexing ($O(1)$ lookup).
  - $O(1)$ swap-and-pop deallocation keeping component arrays packed without holes.
- **Entity Identity & Handle Safety**:
  - 64-bit scalar `Tether_GUID` handles (index + generation) checked in $O(1)$ via `tether_ecs_is_valid()`.
- **Decoupled Spatial & Visual Transforms**:
  - Separation of layout space (`SlotTransform`: $x, y, w, h$) from presentation (`RenderTransform`: translation, scale, rotation, pivot, opacity). Shape/opacity animations skip layout reflow, while text transforms rebuild CPU-side glyph geometry via `DIRTY_TEXT_CONTENT`.
- **Reactive Invalidation & Dirty-Most Parent Root Finding**:
  - Dirty tags (`DIRTY_LAYOUT`, `DIRTY_VISUAL`, `DIRTY_HIERARCHY`, `VOLATILE`) skipping unnecessary sync work when static (`tether_did_draw() == false`).
  - **Dirty-Most Parent Root Finding**: Traverses upward through parent links to evaluate the highest dirty layout root once per subtree rather than re-solving descendants repeatedly.
- **Modular Widget Suite (`Container`, `Panel`, and 13 Built-In Widgets)**:
  - `Text`, `Button`, `ProgressBar`, `Slider`, `Toggle`, `Checkbox`, `Radio`, `SegmentedBar`, `Menu`, `Dropdown`, `ContextMenu`, `Collapsible`, and `Switcher`, plus logical `tether_list` selection containers, `TETHER_POSITION_ABSOLUTE`/`FIXED` overlay Z-ordering, and disabled subtree gating (`is_enabled`).
  - Small String Optimization (SSO) tiers (`TextWord` 32B, `TextLabel` 128B, `TextParagraph` 512B) inline in ECS storage, plus heap-backed `TextDynamic` for long strings.
- **Central Registry System**:
  - Entity string alias lookup (`tether_registry_find_entity`), dynamic widget registration (`tether_register_widget()`), and font ID management.
- **Declarative YAML Parser & Format-Neutral AST**:
  - `tether_yaml_load()` / `tether_yaml_term()` building `Tether_ASTNode` trees with `$var` / `$slot` templates, recursion depth caps (`64`), and failure-path entity cleanup (manual reload in Sandbox; automatic file-watched reload is future work, and YAML exposes a subset of C APIs).
- **Chitra GPU Renderer**:
  - Signed Distance Field (SDF) rounded-rectangle/border shaders, Fontstash dynamic font glyph cache, hierarchical scissor clipping (`clip_id = 0` overlay escape), and 64-bit key radix sorting.
- **Normalized Input & Spatial Hit-Testing**:
  - Reverse depth-first search (DFS) with priority input routes (`tether_input_set_route`), disabled-subtree culling, right-click bubbling, and `ABSOLUTE`/`FIXED` overlay traversal.
- **Event & Entity Binding**:
  - Reentrant-safe `tether_bind_event()`, `tether_bind_pointer_event()`, `tether_unbind_event()`, and `tether_unbind_callback()`, paired with `tether_ui_set_text()` / `tether_ui_set_color()` updates.

### 2. Active Integration & Roadmap Milestones
- **Vinyasa Layout & Strategy Registry**: Integrating the standalone stateless constraint engine and `tether_layout_register_strategy()` extensibility.
- **Ahead-of-Time (AOT) Bytecode Cooker (`.tbc`)**: Compiling YAML scenes into binary `.tbc` assets for instant cold boot on embedded targets.
- **Zero-Allocation Audit & Embedded SRAM Validation**: Profiling remaining frame allocations (`TextDynamic`/pool growth) and validating sub-64 KB bare-metal drivers.
- **Unicode Complex Text Shaping**: Optional HarfBuzz/FreeType backend adapters for RTL/bidirectional scripts and contextual ligatures.
- **2D Affine Inverse Hit-Testing & Cross-OS Expansion**: Inverse-transform hit detection, text input widgets, and CI validation across Linux, macOS, and WASM.

---

## 3. Deliberate Engineering Trade-offs

Every major architectural decision in Tether involves a conscious trade-off:

### 1. Single-Threaded UI Main Loop vs. Multi-Threaded Locking
- **The Decision**: Tether's core engine, layout solver, and visual synchronizer execute on a single dedicated UI thread without fine-grained mutex synchronization.
- **Strategic Rationale**: In a Data-Oriented ECS, performance is achieved by sweeping through contiguous memory arrays at memory bus bandwidth speeds. Introducing mutexes or read-write locks across individual component arrays destroys CPU L1/L2 cache prefetching, introduces severe lock contention, and creates complex deadlock hazards.
- **The Trade-off**: Background worker threads cannot mutate Tether ECS components directly. They must marshal state changes onto the UI thread (`on_frame` / application queues).

### 2. Retained-Mode State vs. Immediate-Mode Discard
- **The Decision**: Tether preserves the UI scene graph across frames in contiguous component tables, governed by fine-grained dirty flags.
- **Strategic Rationale**: Immediate-mode UI frameworks discard widget structure every frame, re-running widget submission continuously. Retained-mode preserves layout/spatial state so static frames skip redundant work (`tether_did_draw() == false`) and provide persistent hit-testing targets.
- **The Trade-off**: Retained-mode maintains component pools between frames, and growing capacity pools or `TextDynamic` allocate dynamically until tuned/reclaimed, whereas immediate-mode discards nearly all persistent widget state per frame. Total idle power still depends on how the host event loop sleeps between frames.

### 3. SDF Shaders vs. Pre-Tessellated Meshes
- **The Decision**: Chitra renders UI cards, rounded rectangles, and borders procedurally using SDF fragment shaders on quad geometry.
- **Strategic Rationale**: Traditional UI engines tessellate curved geometry into many triangle vertices, increasing CPU/vertex traffic. SDF shaders deliver crisp, resolution-independent rounded boxes and borders directly per pixel.
- **The Trade-off**: While SDF shaders excel for UI primitives, rendering arbitrary complex SVG vector paths requires specialized vector rasterizer backends (such as ThorVG).

### 4. 64-Bit Scalar GUIDs vs. Raw Entity Pointers
- **The Decision**: Host applications, foreign language bindings, and scripts identify entities via 64-bit scalar values (`Tether_GUID`) rather than raw `Entity*` pointers.
- **Strategic Rationale**: Exposing raw entity pointers across language boundaries risks dangling pointers and use-after-free bugs when elements are destroyed. A 64-bit generational GUID allows $O(1)$ safety validation (`tether_ecs_is_valid()`).
- **The Trade-off**: Component access performs an $O(1)$ sparse-map lookup instead of a direct pointer dereference, and C callers holding pointers returned by `tether_ecs_get_component()` must re-query them after ECS mutations that resize or swap dense arrays.

---

## 4. Active Architectural Limitations

Tether is actively evolving. Evaluators should be aware of the following current technical limitations:

| Architectural Area | Current Behavior | Strategic Context & Planned Evolution |
| :--- | :--- | :--- |
| **Spatial Hit-Testing** | Evaluates un-transformed layout space (`SlotTransform`). | Hit-testing currently queries the un-rotated, un-scaled bounding box generated by layout. If an element has an extreme visual rotation or scale applied via `RenderTransform`, hit bounds reflect the original layout slot. Implementation of full 2D affine inverse-matrix projection is planned for a future update. |
| **Corner Radius Rendering** | ECS stores per-corner `Tether_Edges`; Chitra draws uniform radius. | While `Tether_Shape` carries four floats (`Tether_Edges`), Chitra's current vertex format sends a single corner radius applied uniformly across all four corners. Asymmetric per-corner rounding is scheduled for future shader upgrades. |
| **Platform & Input Scope** | Windows CI validated; pointer-primary interactions. | `v0.0.3-preview` validates Windows (`MSVC` & `Clang ASan`) with Chitra/Sokol and pointer-based widgets; editable text fields, full keyboard navigation, and Linux/macOS/MCU CI remain roadmap milestones. |
| **Complex Typography Shaping** | Backend glyph caching via Fontstash. | Ideal for Western and Latin typography, monospace code, and standard symbols. Complex non-Latin scripts requiring contextual ligature shaping and bidirectional text (such as Arabic, Hebrew, or Devanagari) require integration with an external shaping engine such as HarfBuzz. |
| **Dynamic Component IDs** | Dynamic ID lookup above compile-time bounds. | Core components use compile-time enums (`0` to `TETHER_COMPONENT_MAX - 1`) with direct array indexing. Third-party components registered dynamically at runtime incur a small dynamic lookup penalty. |

---

## 5. Practical Decision Framework: Applicability (Current Preview vs. Roadmap)

Tether targets native desktop tools and host-driven loops today (`v0.0.3-preview` validated on Windows), while architecting clean seams toward broader computing domains:

### 1. Desktop Creative Tools, DAWs, and Utilities (`Current Preview Focus`)
- Native C99 core with declarative YAML authoring (`tether_yaml_load`), Chitra GPU rendering, and retained dirty-flag invalidation.
- Avoids multi-hundred-megabyte Chromium/Electron runtimes while delivering native startup and manual YAML scene reload in the Sandbox.

### 2. Game Engine HUDs and Embedded Host Loops (`Supported via tether_on_* Hooks`)
- Host engines drive Tether directly via `tether_on_init`, `tether_on_input`, `tether_on_frame`, and `tether_on_cleanup` alongside custom `Tether_Backend` adapters.
- Zero tracing GC pauses and packed dense arrays suit tight game frame budgets (target sub-millisecond profiling via `tether_bench`).

### 3. Developer Tooling and Runtime Overlays (`Active Preview + Expanding`)
- Combines retained two-pass layout with declarative YAML scene composition and a 13-widget interactive suite without immediate-mode tree re-submission every frame (`automatic file-watched hot-reload` is future work).

### 4. Web Applications via WebAssembly (`Roadmap Target`)
- Designed to compile cleanly to WASM via Emscripten over Sokol/WebGPU or WebGL2 without DOM churn or JS GC pauses once WASM CI and backend wiring land.

### 5. Embedded Systems and Bare-Metal Microcontrollers (`Roadmap Target`)
- Pipeline 1 (Pure C) with static component capacities and custom framebuffer drivers targets constrained ARM Cortex-M, ESP32, and RISC-V displays (sub-64 KB SRAM validation pending).

### 6. Long-Lived Systems Software (`Architectural Strength`)
- Anchored to standard ISO C99 (`CMAKE_C_STANDARD 99`) with minimal vendored build dependencies (`libyaml`, `chitra`), reducing external framework churn over multi-decade lifespans.

---

[<- Previous: Chapter 08 - Demonstrations and Verified Performance](08_demonstrations_and_benchmarks.md) | [Documentation Master Index](../index.md) | [Next: Chapter 10 - Future Roadmap and Community Participation ->](10_roadmap_and_community.md)
