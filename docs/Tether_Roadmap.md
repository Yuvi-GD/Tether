# Tether UI: Master Execution Roadmap

This roadmap is structured by **Verifiable Outcomes**. Under each outcome, the exact C-level files, structs, and functions required to make the system work are detailed so you can understand the precise programming steps.

---

## Outcome 1: The UI-ECS Kernel (Memory Foundation) ✅ COMPLETED
**Goal:** Prove that we can manage 10,000 UI elements in pure C without a Garbage Collector and with zero memory fragmentation.
**Verification:** A C-console test that spawns 10k entities, deletes 5k, and proves the memory array remains 100% packed.

**Technical Details (What we built):**
- **File:** `tether_ecs.h`, `tether_ecs.c`
- **Structs:**
  - `typedef uint64_t Tether_GUID;` (32-bit ID + 32-bit Generation versioning).
  - `typedef struct Tether_SparseMap { uint32_t* dense_indices; uint32_t capacity; }`
  - `typedef struct Tether_DenseArray { void* data; size_t element_size; uint32_t count; uint32_t capacity; }`
- **Functions:**
  - `Tether_GUID tether_ecs_create_entity(void);`
  - `void tether_ecs_destroy_entity(Tether_GUID entity);` (Executes the Swap-and-Pop math to keep `DenseArray` packed).
  - `void* tether_ecs_add_component(Tether_GUID entity, int component_type);`

---

## Outcome 2: The YAML Parser (The Input Pipeline) ✅ COMPLETED
**Goal:** Prove that we can feed data into the ECS engine using a simple text file, eliminating the need to hardcode C-structs.
**Verification:** Write `index.yaml`, run the C engine, and print out the populated ECS arrays.

**Technical Details (What we built):**
- **File:** `tether_components.h`
  - Component definitions and IDs.
  - `struct Tether_Hierarchy { Tether_GUID parent, first_child, next_sibling; uint32_t child_count; }`
- **File:** `parsers/tether_yaml.h`, `parsers/tether_yaml.c`
  - Embedded `libyaml` for fast parsing directly into ECS component arrays.
- **Functions:**
  - `Tether_GUID tether_yaml_load(const char* filepath);`

---

## Outcome 3: The Rasterizer Bridge (The Visual Output) ✅ COMPLETED
**Goal:** Complete the loop. YAML goes in -> ECS processes it -> Pixels come out via ThorVG.
**Verification:** Run `index.yaml`. The window opens, and ThorVG draws the exact colored boxes described in the YAML onto the WebGPU canvas.

**Technical Details (What we built):**
- **File:** `tether_components.h` (additions)
  - `struct Tether_Color { uint8_t r, g, b, a; }`
- **File:** `backends/tether_raster.h`, `backends/raster/thorvg_raster.c`
- **Functions:**
  - `void tether_raster_draw(void);` (Loops linearly over `SlotTransform`, `RenderTransform`, and `Color` dense arrays).

---

## Outcome 4: The Layout Engine (The Math) ✅ COMPLETED
**Goal:** Prove that the Engine can calculate complex layouts (Canvas/Anchor & Flexbox/Rows/Columns) dynamically.
**Verification:** A YAML file with nested Box and Canvas flows automatically calculates absolute positions and draws evenly.

**Technical Details (What we built):**
- **File:** `tether_components.h` (additions)
  - `struct Tether_SlotTransform` (Calculated absolute layout boundary).
  - `struct Tether_RenderTransform` (High-frequency translation/scale/rotation offsets).
  - `struct Tether_Layout`, `Tether_AnchorSlot`, `Tether_FlexSlot` (Layout Rules).
  - `struct Tether_RenderTransform` (Matrix and Visual bounds).
  - `struct Tether_Style` (Backgrounds, borders, corners).
  - `struct Tether_Text` (Typography, alignment, overflow handling).
  - `struct Tether_Hierarchy` (O(1) DOM traversal).
  - `struct Tether_Interactable` (Hover state, focus, hit-testing).

### Implementation Details:
- **Phase 1: Component Refactor & Visibility (DONE)**
  - Split `Tether_Text` from `Tether_Style`.
  - Add `Tether_Id` component and `TETHER_COMPONENT_IS_LEAF` flag to prevent erroneous child attachments.
  - Implement `opacity` on `Tether_RenderTransform`.
  - Migrate hit-testing rules, caching, and state transitions to `Tether_Interactable`.
  - Update YAML parser to resolve IDs, explicit text colors, tags, and opacity properly.

- **Phase 2: Layout Node Split (IN PROGRESS)**
  - Rename `Tether_LayoutNode` to `Tether_Layout`.
  - Move `explicit_size` (renamed `size_box`) from FlexSlot to `Tether_Layout`.
  - Add `pivot` point property to `Tether_AnchorSlot`.
  - Phase out `override_align` flags from `Tether_FlexSlot` using `TETHER_ALIGN_AUTO`.
  - `hover_color_mode`, `hover_color`, `press_color_mode`, `press_color` on `Tether_Style`. Supports `AUTO` (±30 brightness) and `MANUAL` (explicit RGBA).
- **File:** `tether_events.h`, `tether_events.c` [NEW]
  - `Tether_EventType` enum: `HOVER_ENTER`, `HOVER_EXIT`, `PRESS`, `RELEASE`, `CLICK`.
  - `tether_bind_event(entity, type, callback, user_data)` — bind any C-function to any entity event.
  - `tether_dispatch_event(entity, type)` — called internally by the input engine.
  - **Functions:**
  - `void tether_layout_process_all(float screen_width, float screen_height);`
  - `void tether_layout_process_tree(Tether_GUID root, float screen_width, float screen_height);` (Traverses hierarchy to calculate layout bounds).

---

## Outcome 5: The Interaction Engine (Input & Hit Testing) ✅ COMPLETED
**Goal:** Prove that the UI is actually interactive, not just a static image.
**Verification:** Hovering and clicking Panels triggers real-time color changes. Clicking the Settings button fires a C-callback. All without touching the rendering code.

**Technical Details (What we built):**
- **File:** `tether_components.h` (additions)
  - `Tether_HitBehavior` enum (`BLOCK`, `IGNORE_SELF`, `IGNORE_ALL`) on `Tether_Layout`. Panels default to `BLOCK`, Text defaults to `IGNORE_SELF`.
  - `char id[32]` on `Tether_Layout` for YAML-defined entity naming.
  - `hover_color_mode`, `hover_color`, `press_color_mode`, `press_color` on `Tether_Style`. Supports `AUTO` (±30 brightness) and `MANUAL` (explicit RGBA).
- **File:** `tether_events.h`, `tether_events.c` [NEW]
  - `Tether_EventType` enum: `HOVER_ENTER`, `HOVER_EXIT`, `PRESS`, `RELEASE`, `CLICK`.
  - `tether_bind_event(entity, type, callback, user_data)` — bind any C-function to any entity event.
  - `tether_dispatch_event(entity, type)` — called internally by the input engine.
- **File:** `tether_input.h`, `tether_input.c` (rebuilt)
  - `tether_hit_test(x, y)` — **Reverse Depth-First Search** over the hierarchy for Z-Index accurate hit resolution. **O(1) Spatial Culling** skips entire branches when the mouse is outside a parent's `SlotTransform`.
  - `tether_input_process_event(event)` — stateful pointer processor managing `hovered_entity` and `pressed_entity` transitions, automatically dispatching events.
- **File:** `tether_ecs.c` (addition)
  - `tether_ecs_find_by_id(const char* id)` — scan the `LayoutNode` dense array to find an entity by its YAML id string.
- **File:** `tether.h`
  - `void (*on_init)(void)` lifecycle callback on `Tether_App_Config` for binding events after YAML is loaded.
- **YAML:** `interactable: true`, `id:`, `hit_behavior:`, `hover_color:`, `press_color:` all fully parsed.

---

## Outcome 6: Advanced Composition (Unreal-Style Hierarchy)
**Goal:** Prove the Unreal Slate compositional model works. A Button is an Interactable panel that *holds* a separate Text element.
**Verification:** A YAML file defining a `Button` containing a `Text` child renders perfectly and handles hover states.

**Technical Details (What we will build):**
- **File:** `tether_components.h` (additions)
  - `struct Tether_Text { char* string; float font_size; uint32_t font_id; }`
- **File:** `tether_render.c` (additions)
  - Add text-rendering dispatches to ThorVG using the `Tether_Text` array.
- **File:** `tether_yaml.c` (additions)
  - Update the parser to natively recognize `Button:` as a macro that creates a Panel with an `Interactable` component.

---

## Outcome 7: The Data Binding Bridge (Push-Events)
**Goal:** Prove that the UI logic is strictly decoupled from the Host language.
**Verification:** A C-loop increments a `Score` integer. The UI Text automatically updates without the C-loop touching any rendering code.

**Technical Details (What we will build):**
- **File:** `tether_binding.h`, `tether_binding.c`
- **Structs:**
  - `struct Tether_BindingRegistry { char* key; void* host_memory_ptr; }`
- **Functions:**
  - `void Tether_Bind(const char* key, void* host_memory_ptr);` (Host registers the variable).
  - `void Tether_MarkDirty(const char* key);` (Host pushes the update flag to Tether).

---

## Outcome 9: Modern UI Rendering (Aesthetics)
**Goal:** Prove Tether can render high-end modern UI designs matching CSS/Figma capabilities.
**Verification:** A complex YAML file rendering rounded borders, drop shadows, gradients, and clipped scrolling areas.

**Technical Details (What we will build):**
- **Borders & Radii**: Stroke width, color, and rounded corners (with content clipping).
- **Shadows**: Outer drop shadows, inner shadows, and glows.
- **Gradients**: Linear and radial gradients.
- **Glassmorphism**: Backdrop blur and frosted glass effects.
- **Images**: PNG, JPG, SVG, and Lottie animations.
- **Phase 2 - Overflow Control System**: Implementation of an Overflow component (`WRAP`, `CLIP`, `SCROLL`, `VISIBLE`) allowing parent containers to dynamically handle child layout boundaries and clipping.

---

## Outcome 11: UI Prefabs and Macros (Slate/UMG Composition)
**Goal:** Prove the engine can parse high-level component macros (like a "Dropdown" or "Slider") that instantly expand into a full hierarchy of raw ECS primitives (Panels, Text, Borders).
**Verification:** A YAML file defines `- Button:` and the engine automatically generates the Panel, the Hit Behavior, the Interactable component, and the Text child without the user writing out the raw hierarchy.

**Technical Details (What we will build):**
- **Macro Expansion Engine:** A parser step that intercepts known composite names (`Button`, `Dropdown`, `Checkbox`) and injects the pre-configured ECS entity templates.
- **Prefab System:** The ability for users to define custom YAML templates and instance them across their UI.

---

## Outcome 10: The Production Deployment Pipeline
**Goal:** Prove Tether can be deployed to NASA hardware or iOS devices bypassing string parsing.
**Verification:** Delete the YAML parser from the app, load a raw `.tbc` binary file, and prove the UI still renders perfectly.

**Technical Details (What we will build):**
- **File:** `tether_cooker.c` (Standalone executable)
  - `void compile_yaml_to_tbc(const char* yaml_file, const char* out_file);`
- **File:** `tether_interpreter.h`, `tether_interpreter.c`
  - `void tether_load_tbc(const char* tbc_filepath);` (Reads raw bytecode directly into ECS memory).
- **File:** `tether_dynamic.c`
  - `void tether_load_plugin(const char* dll_path);` (For Windows/Linux/Android dynamic primitives).

---

## Outcome 12: Developer Experience (Zero-Friction Build & Rendering)
**Goal:** Ensure Tether can be dropped into any project instantly by making heavy dependencies (WebGPU) strictly opt-in, and providing a clean toggle between pure CPU and native GPU APIs.
**Verification:** A developer can clone the repo and `cmake .` instantly without downloading 40MB of binaries, yielding a tiny OpenGL binary. They can then set `TETHER_GPU_API=WGPU` and get the full next-gen pipeline.

**Technical Details (What we will build):**
- **File:** `CMakeLists.txt`
  - Add `TETHER_GPU_API` option (GL or WGPU). Wrap `wgpu-native` FetchContent in an `if` block. Switch ThorVG's meson flag to `-Dengines=gl` or `-Dengines=wg`.
- **File:** `sokol_hal.c`
  - Add `#ifdef TETHER_GPU_GL` vs `TETHER_GPU_WGPU`. Setup `SOKOL_GLCORE33` for GL. Bypass offscreen texture blitting if GL is active (render directly to screen).
- **File:** `thorvg_raster.c`
  - Initialize the correct ThorVG engine (`TVG_ENGINE_GL` or `TVG_ENGINE_WG`) based on compile flags.
- **File:** `tether.h`
  - Add `Tether_Render_Mode` to `Tether_App_Config` with `TETHER_RENDER_HARDWARE` (uses selected API) and `TETHER_RENDER_SOFTWARE` (initializes ThorVG's `sw` engine for 100% pure CPU rendering).

---

## Outcome 13: The ECS ID Hash Map (O(1) String Lookup)
**Goal:** Optimize `tether_ecs_find_by_id` to prevent O(N) string iteration bottlenecks during entity lookup.
**Verification:** Implement a basic string-to-GUID Hash Map (e.g. Robin Hood hashing) so developers can query YAML `id` tags in constant time without looping over the Dense Array of tags.

**Technical Details:**
- **File:** `tether_ecs.h`, `tether_ecs.c`
  - Implement a `Tether_HashMap` for String -> `Tether_GUID`.
  - Update `tether_ecs_add_component` for `TETHER_COMPONENT_TAG` to push to the hash map.
  - Update `tether_ecs_find_by_id` to query the hash map instead of looping.

---

## Outcome 15: Core ECS Extensibility (Dynamic Components)
**Goal:** Prove developers can define and register custom components dynamically without core engine modifications.
**Verification:** A developer registers a custom component via `tether_ecs_register_component`, and the engine safely expands its bounds and assigns a safe ID automatically (e.g. 19, 20) without relying on hardcoded `#define TETHER_COMPONENT_MAX`.

**Technical Details:**
- **File:** `tether_ecs.c`
  - Implement dynamic `g_next_component_id` tracking.
  - Implement `uint32_t tether_ecs_register_component(size_t element_size)` that auto-increments and returns a safe ID.

---

## Outcome 14: Container-Agnostic Layout Architecture
**Goal:** Prove the layout engine can process new container types (like Grid or Wrap) without modifying core engine source code.
**Verification:** A Layout Strategy Registry is created, and Flex/Canvas logic is decoupled from `tether_layout.c` using registered layout function pointers.

**Technical Details:**
- **File:** `tether_layout.h`, `tether_layout.c`
  - Implement `Tether_LayoutStrategy` struct and registry.
  - Implement `tether_layout_register_strategy(uint32_t comp_id, measure_fn, arrange_fn)`.
  - Refactor `arrange_top_down` and `measure_bottom_up` to iterate over the registry instead of hardcoding `if (flow)`.
