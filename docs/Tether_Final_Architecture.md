# Tether: Final Architecture

> **Tether is not a UI library. Tether is an engine for building UI libraries.**
>
> Just as LLVM provides the infrastructure to build compilers (instead of being one monolithic compiler like GCC), Tether provides the infrastructure to build UI systems — from NASA mission control dashboards to mobile apps to game HUDs.

---

## The Guiding Principle: The Tree Branch Model

Tether is organized into a **3-Layer Tree Branch Architecture**. Unlike a traditional sequential stack (where Layer 2 sits on top of Layer 1 and calls into it), Tether's Layer 1 contains **three isolated foundational pillars** that never communicate directly. The Engine (Layer 2) is the trunk that bridges them together, and the UI Library (Layer 3) is where concrete widgets and rules are defined on top.

```
                         ┌─────────────────────────────────────────┐
                         │           Layer 3: UI Library           │
                         │ Canvas, Row, Column, Button, Text,      │
                         │ Custom Widgets... (Rules & Widgets)     │
                         └───────────────────┬─────────────────────┘
                                             │
                         ┌───────────────────▼─────────────────────┐
                         │              Layer 2: Engine            │
                         │ Layout, Input/Events, Registry, Parsers │
                         │              (Systems & Math)           │
                         └─────────┬───────────────┬───────────────┘
                            │                │                │
            ┌───────────────▼───┐  ┌─────────▼─────────┐  ┌───▼────────────────┐
            │ Layer 1: Kernel   │  │ Layer 1: HAL      │  │ Layer 1: RHI       │
            │ ECS memory        │  │ OS, window, input │  │ Pixels / rendering │
            └───────────────────┘  └───────────────────┘  └────────────────────┘
```

**Dependency Rule:** Layer 3 depends on Layer 2. Layer 2 depends on all three Layer 1 pillars. The three Layer 1 pillars are entirely isolated — the Kernel never talks to the HAL, and the HAL never talks to the RHI directly.

---

## Layer 1: The Foundations

This layer contains the three base pillars of the engine. They are entirely independent, know nothing about UI rules, and never communicate with each other.

---

### 1A. The Kernel (ECS Memory)

#### Purpose
The Kernel is a pure, general-purpose **Entity Component System** written in C. It is the memory backbone of the entire engine. It knows absolutely nothing about UI, hierarchies, strings, screens, or rendering. It only manages raw memory.

#### Core Concepts
- **Entity:** Just a 64-bit integer ID (a `Tether_GUID`). Lower 32 bits are a recycled array index, upper 32 bits are a generation version that prevents use-after-free bugs when indices are recycled.
- **Component:** An arbitrary blob of bytes. The Kernel does not know what a `Tether_Style` or `Tether_Hierarchy` is — it only stores `element_size` bytes of data and associates them with an entity.
- **Dense Array:** The actual memory where components live. All components of the same type are stored perfectly contiguous in RAM. When iterating (e.g., to draw all panels), the CPU sweeps one flat array with zero pointer chasing — maximum cache locality.
- **Sparse Map:** Maps an Entity Index to its position in a Dense Array. This gives $O(1)$ lookups: given an entity and a component type, we can instantly find the data.
- **Swap-and-Pop:** When an entity is destroyed, its component data is swapped with the last element in the Dense Array and then popped. This keeps the array perfectly packed with zero fragmentation.

#### Core Data Structures
```c
/* 64-bit ID: Lower 32 = Index, Upper 32 = Generation */
typedef uint64_t Tether_GUID;
#define TETHER_INVALID_GUID 0

/* Maps Entity Index → Dense Array Index */
#define TETHER_SPARSE_INVALID_INDEX 0xFFFFFFFF
typedef struct Tether_SparseMap {
    uint32_t* dense_indices;
    uint32_t  capacity;
} Tether_SparseMap;

/* A perfectly contiguous block of component data */
typedef struct Tether_DenseArray {
    void*         data;          /* Raw block of struct memory */
    Tether_GUID*  entity_map;    /* Maps Dense Index → Entity GUID (reverse lookup) */
    size_t        element_size;  /* Size of one component in bytes */
    uint32_t      count;         /* Number of active elements */
    uint32_t      capacity;      /* Allocated capacity */
} Tether_DenseArray;
```

#### Core API
```c
/* --- Lifecycle --- */
void tether_kernel_init(void);   /* Allocate the global ECS registry */
void tether_kernel_term(void);   /* Free all memory */

/* --- Entity Management --- */
Tether_GUID tether_entity_create(void);            /* Create a raw entity (just an ID) */
void        tether_entity_destroy(Tether_GUID e);   /* Destroy entity + all its components via Swap-and-Pop */
bool        tether_entity_is_valid(Tether_GUID e);  /* Check if entity ID is still alive (generation check) */

/* --- Component Registration (ID Space Reservation) ---
 * To maximize performance while preventing collisions, Tether uses grouped boundaries:
 *   0 to 5: Core Engine Components
 *   6 to 16: UI System Components
 *   17 to 48: User Hardcoded Components
 *   49+: Dynamic Third-Party Components
 * 
 * Internal and hardcoded user components use macros for $O(1)$ compile-time lookups.
 * Dynamic plugins use tether_component_register() to safely get collision-free IDs. */

/* Register a hardcoded internal/user component */
void tether_ecs_register_component_static(uint32_t component_id, size_t element_size);

/* Register a dynamic third-party component (returns auto-incremented ID) */
uint32_t tether_ecs_register_component_dynamic(size_t element_size);

/* --- Component Access --- */
void*            tether_component_add(Tether_GUID e, uint32_t comp_id);    /* Attach memory block to entity */
void*            tether_component_get(Tether_GUID e, uint32_t comp_id);    /* O(1) lookup via Sparse Map */
void             tether_component_remove(Tether_GUID e, uint32_t comp_id); /* Swap-and-Pop removal */
Tether_DenseArray* tether_component_get_array(uint32_t comp_id);           /* Raw array for linear iteration */
```

#### The Small String Optimization (SSO) Text Architecture
To maintain pure Cache Locality and prevent heap fragmentation, text is stored directly inside fixed-size struct components:
- `Tether_TextWord` (32 bytes) — short labels like "OK", "Cancel"
- `Tether_TextLabel` (128 bytes) — button/menu text
- `Tether_TextParagraph` (512 bytes) — descriptions, tooltips
- `Tether_TextDynamic` (heap-allocated, geometric growth) — code editors, chat logs

The setter function `tether_ecs_set_text_string()` automatically picks the smallest component that fits the string.

#### Key Files
```
include/tether/kernel/
    tether_ecs.h              ← Public Kernel API + data structures

src/kernel/
    tether_ecs.c              ← Implementation (Sparse Set, Dense Arrays, Swap-and-Pop)
```

---

### 1B. The HAL (Hardware Abstraction Layer)

#### Purpose
The HAL is the bridge between Tether and the Operating System. It handles windowing, the main application loop, and raw hardware input capture. The engine above this layer never knows if it is running on Windows, Linux, macOS, or even a headless test harness.

#### Core Responsibilities
- Open the OS window with a title, width, and height.
- Manage the main application loop (supports VSync, uncapped FPS, or a target framerate).
- Set up the GPU context (e.g., WebGPU device and instance) and pass it to the RHI.
- Capture raw OS events (mouse movement, clicks, key presses, window resize) and normalize them into Tether's `Tether_Pointer_Event` struct before handing them to the Engine.

#### Core Structures
```c
typedef enum {
    TETHER_MEMORY_HIGH_WATER_MARK = 0,  /* Keep allocated memory for max speed */
    TETHER_MEMORY_AGGRESSIVE_SHRINK     /* Shrink memory back to OS on entity deletion */
} Tether_Memory_Strategy;

typedef enum {
    TETHER_RENDERER_WEBGPU = 0,         /* Hardware accelerated via ThorVG/WebGPU */
    TETHER_RENDERER_SOFTWARE_CPU        /* Pure CPU rasterization (future) */
} Tether_Renderer_Backend;

typedef struct {
    int         width;
    int         height;
    const char* title;
    const char* initial_yaml;

    /* Engine Behavior Configuration */
    Tether_Memory_Strategy  memory_strategy;
    Tether_Renderer_Backend renderer;
    int target_fps;  /* 0 = VSync (match monitor refresh rate) */

    /* Lifecycle Callbacks */
    void (*on_init)(void);  /* Called after YAML is loaded, before the first frame */
} Tether_App_Config;
```

#### Core API
```c
/* Start the OS windowing loop. This function takes over the main thread.
 * Internally it drives the per-frame pipeline:
 *   1. Poll OS events → convert to Tether_Pointer_Event → pass to Engine Input System
 *   2. Call Engine Layout System to recalculate positions
 *   3. Call RHI to render the frame
 *   4. Present the frame to the display */
void tether_hal_run(Tether_App_Config* config);
```

#### Compile-Time Backend Swapping
The HAL uses compile-time swapping. To replace the windowing backend (e.g., from Sokol to SDL), you write a new `.c` file that implements the functions declared in `tether_hal.h` and update `CMakeLists.txt` to compile it instead.

#### Key Files
```
include/tether/backends/
    tether_hal.h              ← HAL function declarations (the "contract")

src/backends/
    sokol_hal.c               ← Concrete implementation using Sokol App
```

---

### 1C. The RHI (Render Hardware Interface)

The Rendering layer (Layer 1) relies on a scalable, modular, and Retained-Mode architecture to maximize performance and decouple the UI logic from graphics APIs (ThorVG, WebGPU, OpenGL).

## 1. The Render Command Buffer
The RHI backend must **never** read ECS components directly. Instead, it consumes a standardized **Render Command Buffer (Draw List)**.
- **Engine Layer**: Systems (e.g., UI, Custom Charts) iterate through their specific ECS components and push generic primitive instructions (`DRAW_RECT`, `DRAW_TEXT`, `DRAW_PATH`, `PUSH_CLIP`) to a contiguous C-array.
- **RHI Layer**: The backend (`tether_rhi.h`) loops over this dense array and translates it into backend-specific calls (e.g., `tvg_shape_append_rect` for CPU or pushing vertex buffers for GPU). 
- **Benefit**: This guarantees seamless cross-API compatibility. The backend only understands mathematical shapes, while the ECS components remain pure data. It naturally supports vectors, images, clipping, and transformations without any rendering logic bleeding into the core engine.

## 2. Dirty Flags (Stateless UI Updates)
Tether uses a fully **Retained-Mode** rendering strategy, meaning the backend scene graph (or GPU buffers) is preserved across frames.
- When an entity's data changes (e.g., color shift, hover event, layout change), the engine assigns it a `TETHER_DIRTY_VISUAL` flag.
- During the render phase, the RHI iterates through the ECS tree. It skips any entity without a dirty flag.
- For dirty entities, the RHI recalculates and updates the specific node/buffer, and then **immediately clears the flag**.
- **Benefit**: Zero historical diffing is required. Static UI elements cost almost zero CPU time, rendering instantly from the retained buffers.

## 3. Volatile Mode (Continuous Rendering)
For highly dynamic entities that change every frame (e.g., video players, running stopwatches, spinning loaders, complex particle animations), using transient dirty flags is inefficient.
- These entities are assigned a permanent `TETHER_VOLATILE` flag.
- The RHI will rebuild their rendering buffers every single frame without fail, and the flag is **never removed**.
- **Benefit**: Allows the engine to perfectly optimize 99% of static UI while dedicating performance exclusively to active, moving elements.


#### Purpose
The RHI is the graphics drawing API. It communicates with the GPU (or CPU) to turn raw mathematical coordinates, shapes, text, and colors into pixels on the screen. It is powered by **ThorVG** (a lightweight vector graphics engine) and renders onto a WebGPU texture.

#### Core Responsibilities
- Initialize the graphics context using the GPU device/instance passed from the HAL.
- Handle window resize events.
- **Draw shapes:** Given `SlotTransform` (x, y, width, height) and `Style` (colors, borders, radii), draw filled rectangles, rounded rectangles, and bordered shapes.
- **Draw text:** Given text string data, font ID, font size, and a bounding box, render glyphs using the registered font. Supports word-wrapping when `max_width` is constrained.
- **Draw images:** Given an asset path and bounds, render PNG/JPG/SVG content.
- **Measure text:** Given a string, font, and optional max_width constraint, return the rendered width and height. This is called by the Engine's Layout System to calculate intrinsic sizes.
- **Apply transforms:** Translation, scale, rotation, and pivot offsets from `RenderTransform`.
- **Apply clipping:** When an entity has a `ClipMask`, restrict rendering to the parent's bounds.

#### Core API
```c
/* Initialize graphics context with GPU device from HAL */
void tether_rhi_init(uint32_t width, uint32_t height,
                     const void* device, const void* instance);

/* Handle window resize */
void tether_rhi_resize(uint32_t width, uint32_t height);

/* Execute the draw command (sync retained scene to GPU) */
void tether_rhi_draw(void);

/* Get the rendered texture for HAL to present to the display */
const void* tether_rhi_get_texture(void);

/* Teardown graphics context */
void tether_rhi_term(void);

/* --- Retained Mode Granular Sync API --- */
/* The Engine calls these to construct the RHI scene graph */
void* tether_rhi_create_rect(void);
void* tether_rhi_create_text(void);
void* tether_rhi_create_scene(void);

void tether_rhi_scene_push(void* scene_handle, void* child_handle);
void tether_rhi_scene_remove(void* scene_handle, void* child_handle);
void tether_rhi_add_to_canvas(void* render_handle);

/* Sync Properties */
void tether_rhi_set_rect_geometry(void* handle, float w, float h, float rx, float ry);
void tether_rhi_set_fill_color(void* handle, Tether_Color color);
void tether_rhi_set_text_string(void* handle, const char* str);
void tether_rhi_set_text_font(void* handle, uint32_t font_id, int style, float size);
void tether_rhi_translate(void* handle, float x, float y);

/* Measure text dimension helper */
void tether_rhi_measure_text(const char* text, uint32_t font_id,
                             int font_style, float font_size,
                             float max_width, float* out_w, float* out_h);
```

#### Compile-Time Backend Swapping
Same as the HAL. To switch from ThorVG to Skia, write `skia_rhi.c` implementing these exact function signatures and swap it in CMake.

#### Key Files
```
include/tether/backends/
    tether_rhi.h              ← RHI function declarations (the "contract")

src/backends/
    thorvg_rhi.c              ← Concrete implementation using ThorVG + WebGPU
```

---

## Layer 2: The Engine (Systems & Orchestration)

#### Purpose
The Engine sits on top of all three Layer 1 foundations and bridges them together. It is the **Orchestrator** — it defines the mathematical rules of the UI system, the tree structure, the input state machine, and the registration systems. The Engine defines the **System Components** that are mandatory for any entity to participate in the UI tree, but it does NOT define any specific widget identity (that is Layer 3's job).

---

### The "Base Node" (System Components)

Because the Engine must perform spatial math (layout), Z-index traversal (input), and coordinate passing (rendering), it defines a set of **System Components** that every UI entity must have. These are not optional — they are the mathematical skeleton.

```c
/* Defines the absolute final bounds of an entity on screen.
 * Written by the Layout System, read by the Input System and RHI. */
typedef struct {
    float x;       /* Absolute X position in screen pixels */
    float y;       /* Absolute Y position in screen pixels */
    float width;   /* Computed width */
    float height;  /* Computed height */
} Tether_SlotTransform;

/* High-frequency visual offset applied ON TOP of SlotTransform.
 * Used for animations, transitions, and visual effects without
 * triggering a full layout recalculation. */
typedef struct {
    float translation_x;
    float translation_y;
    float scale_x;      /* Default: 1.0 */
    float scale_y;      /* Default: 1.0 */
    float rotation_deg;  /* Degrees */
    float pivot_x;       /* 0.0 to 1.0. Default: 0.5 (center) */
    float pivot_y;       /* 0.0 to 1.0. Default: 0.5 (center) */
} Tether_RenderTransform;

/* Linked-list tree structure for parent-child relationships.
 * The Engine needs this to traverse the tree for layout (bottom-up + top-down)
 * and for input (reverse depth-first hit testing with spatial culling). */
typedef struct {
    Tether_GUID parent;
    Tether_GUID first_child;
    Tether_GUID last_child;
    Tether_GUID prev_sibling;
    Tether_GUID next_sibling;
    uint32_t    child_count;
} Tether_Hierarchy;

/* Controls whether the entity is rendered and participates in layout. */
typedef enum {
    TETHER_VISIBLE   = 0,  /* Renders + takes up layout space */
    TETHER_HIDDEN    = 1,  /* Does NOT render, but DOES occupy layout space (invisible box) */
    TETHER_COLLAPSED = 2   /* Does NOT render, takes ZERO space (layout ignores it entirely) */
} Tether_VisibilityState;

typedef struct {
    Tether_VisibilityState local_state;
    Tether_VisibilityState computed_state;
} Tether_Visibility;
```

---

### 2.1 The Central Registry

The Central Registry is the mastermind of the Engine. It provides **three distinct types of registration** that make Tether fully modular and extensible.

#### A. ID Registry (String → GUID Hash Map)
When a developer assigns `id: "submit_btn"` in YAML, or registers a string ID from C code, the Engine maps that string to a `Tether_GUID` using a **Robin Hood Hash Map** for $O(1)$ constant-time lookups.

```c
/* Register a string → GUID mapping (called during YAML parsing or from C) */
void tether_id_register(const char* string_id, Tether_GUID entity);

/* Find an entity by its string ID. O(1) via hash map. Returns TETHER_INVALID_GUID if not found. */
Tether_GUID tether_id_find(const char* string_id);
```

#### B. Layout Strategy Registry (Container-Agnostic Layout)
The Layout Engine does NOT hardcode knowledge of specific container types (FlexBox, Canvas, Grid). Instead, it uses a **Layout Strategy Registry** where each container type registers its own mathematical rules.

This is what makes the layout engine container-agnostic. The Engine just orchestrates the tree traversal. The actual math (how a Row distributes space, how a Canvas positions children at absolute coordinates) is defined by the registered strategy functions.

```c
/* Function pointers representing the two passes of layout.
 * These are used not only for containers (Flex, Canvas) but also for
 * leaf content types (Text, Image) that need to report their intrinsic size. */
typedef void  (*Tether_MeasureFn)(Tether_GUID entity, void* strategy_data);
typedef float (*Tether_ArrangeFn)(Tether_GUID entity, void* strategy_data,
                                  float cx, float cy, float cw, float ch);

/* A registered layout strategy, bound to a specific ECS component */
typedef struct {
    uint32_t         component_id;  /* Which component triggers this strategy */
    Tether_MeasureFn measure;       /* Bottom-up: report intrinsic size */
    Tether_ArrangeFn arrange;       /* Top-down: position children within bounds */
} Tether_LayoutStrategy;

/* Register a new layout strategy. Both built-in and third-party use this same API. */
void tether_layout_register_strategy(uint32_t component_id,
                                      Tether_MeasureFn measure,
                                      Tether_ArrangeFn arrange);
```

**How the Engine uses it during layout:**
```c
/* Inside the layout pass, the engine does NOT do if(flex) / else if(canvas).
 * Instead, it loops through the registered strategies: */
bool handled = false;
for (int i = 0; i < g_layout_registry_count; i++) {
    void* data = tether_component_get(entity, g_layout_registry[i].component_id);
    if (data) {
        g_layout_registry[i].measure(entity, data);
        handled = true;
        break;
    }
}
/* Fallback: if no strategy matches, treat as absolute positioning (Canvas default) */
if (!handled) {
    measure_canvas_default(entity);
}
```

**What gets registered at startup (Layer 3 does this):**
```c
/* Container strategies (define how parents position children) */
tether_layout_register_strategy(COMP_FLEX_FLOW, measure_flex, arrange_flex);
tether_layout_register_strategy(COMP_CANVAS_FLOW, measure_canvas, arrange_canvas);

/* Content strategies (define how leaf nodes report intrinsic size) */
tether_layout_register_strategy(COMP_TEXT_STYLE, measure_text, arrange_text);
tether_layout_register_strategy(COMP_IMAGE, measure_image, arrange_image);

/* Future: third-party extensions */
tether_layout_register_strategy(COMP_GRID_FLOW, measure_grid, arrange_grid);
```

**This means:** The `MeasureFn`/`ArrangeFn` pattern works for BOTH containers (Flex, Canvas) AND leaf content (Text, Image). A Text's `measure_text` function calls `tether_rhi_measure_text()` to get the intrinsic string dimensions. An Image's `measure_image` computes height from its aspect ratio. The layout engine treats them all the same way — it just asks the registry.

#### C. Widget Registry (Factories)
Maps human-readable string names to C widget creation functions. This is how the Parser knows how to create a "Button" — it asks the Widget Registry.

```c
/* Signature for a widget factory function */
typedef Tether_GUID (*Tether_WidgetCreateFn)(Tether_GUID parent);

/* Register a widget factory by name */
void tether_widget_register(const char* name, Tether_WidgetCreateFn factory_fn);

/* Instantiate a widget by name (used by the Parser when it encounters "Button:" in YAML) */
Tether_GUID tether_widget_create(const char* name, Tether_GUID parent);
```

Both Tether's built-in widgets AND developer-defined widgets use the exact same registration path. There is no special treatment for built-ins.

---

### 2.2 The Layout System

#### Purpose
The Layout System calculates the absolute `(x, y, width, height)` for every entity in the UI tree. It performs a **strict 2-pass tree traversal** using registered layout strategies.

#### The 2-Pass Pipeline
```
┌────────────────────────────────────────────────────────────────────┐
│  Pass 1: Measure (Bottom-Up)                                      │
│                                                                    │
│  Walk from leaves to root. Each node reports its intrinsic size    │
│  upward. Leaf nodes (Text, Image) query the RHI for measurement.  │
│  Container nodes sum up their children's sizes + gaps + padding.   │
│                                                                    │
│  Example:                                                          │
│    measure(Root)                                                   │
│      → measure(Column)                                             │
│          → measure(Button_A)  ← leaf, returns text size + padding  │
│          → measure(Button_B)  ← leaf, returns text size + padding  │
│          → measure(Text)      ← leaf, calls RHI measure_text      │
│          ← Column sums heights + gaps + padding. Done.             │
│      ← Root done.                                                  │
│                                                                    │
│  Every entity visited exactly once. O(N) total.                    │
├────────────────────────────────────────────────────────────────────┤
│  Pass 2: Arrange (Top-Down)                                        │
│                                                                    │
│  Walk from root to leaves. Each parent node receives constraints   │
│  from ITS parent, then distributes space to its children using     │
│  the registered layout strategy.                                   │
│                                                                    │
│  For Flex containers:                                              │
│    1. Reserve fixed space for children with fill_ratio == 0        │
│       (use explicit_size if set, else intrinsic measured size)     │
│    2. available_space = total - fixed_space - gaps                 │
│    3. Distribute available_space proportionally to fill children   │
│                                                                    │
│  For Canvas containers:                                            │
│    Children use AnchorSlot to calculate position relative to       │
│    parent bounds: anchor_min/max define stretch, offset defines    │
│    insets from anchored edges.                                     │
│                                                                    │
│  Each arrange function returns the actual used height back up,     │
│  allowing parents with auto-height to shrink-wrap their content.   │
├────────────────────────────────────────────────────────────────────┤
│  Content Re-Measure (Height-for-Width)                             │
│                                                                    │
│  After the Arrange pass assigns final widths, leaf nodes with      │
│  width-dependent content (Text, Image) are re-measured:            │
│    - Text: If assigned width < intrinsic width, call RHI           │
│      measure_text with max_width to get wrapped height.            │
│    - Image: Compute height from aspect ratio at assigned width.    │
│                                                                    │
│  The generic interface:                                            │
│    float tether_content_measure_for_width(entity, width)           │
│  is called by the layout engine. The implementation checks for     │
│  known content components (Text, Image) and queries the RHI.       │
│  The layout engine itself has ZERO type-specific checks.           │
└────────────────────────────────────────────────────────────────────┘
```

#### Slot Handling Rules
Each container type owns its own slot component:
- **`Tether_FlexSlot`** is read exclusively by `arrange_flex`. It defines: `margin`, `explicit_size`, `fill_ratio`, `align_self_x/y`.
- **`Tether_AnchorSlot`** is read exclusively by `arrange_canvas`. It defines: `anchor_min/max`, `offset`.
- **Future `Tether_GridSlot`** would be read exclusively by a custom `arrange_grid`.

If a third-party developer creates a `HexagonalGrid` container, they register a component `COMP_HEX_GRID` with its own slot `COMP_HEX_SLOT`. The Engine's layout system will call their registered `arrange_hex` function, which internally reads `HEX_SLOT` from children. Zero engine code modifications needed.

#### Core API
```c
/* Process the entire UI tree with the 2-pass pipeline */
void tether_layout_process(Tether_GUID root, float screen_width, float screen_height);
```

---

### 2.3 The Input & Event System

#### Purpose
Handles the complete pipeline from raw OS events to developer-facing callbacks.

#### The Pipeline: OS Event → Hit Test → State Machine → Dispatch

**Step 1: Raw Event Normalization**
The HAL captures OS-specific events and converts them into a normalized struct:
```c
typedef enum {
    TETHER_POINTER_DOWN = 0,
    TETHER_POINTER_UP,
    TETHER_POINTER_MOVE
} Tether_Pointer_EventType;

typedef enum {
    TETHER_MOUSE_BUTTON_LEFT = 0,
    TETHER_MOUSE_BUTTON_RIGHT,
    TETHER_MOUSE_BUTTON_MIDDLE,
    TETHER_MOUSE_BUTTON_NONE
} Tether_MouseButton;

typedef struct {
    Tether_Pointer_EventType type;
    Tether_MouseButton       button;
    float                    x;
    float                    y;
} Tether_Pointer_Event;
```

**Step 2: Hit Testing**
The Input System runs a **Reverse Depth-First Search** over the Hierarchy tree to find the topmost entity under the pointer.
- **Z-Index Accuracy:** By traversing children in reverse order, the visually "topmost" (last drawn) entity is found first.
- **Spatial Culling:** If the pointer is outside a parent's `SlotTransform` bounds, the entire branch is skipped instantly ($O(1)$ per branch).
- **Hit Behavior:** Each entity can control how it interacts with hit testing:
  - `TETHER_HIT_BLOCK` — catches the hit, stops search.
  - `TETHER_HIT_IGNORE_SELF` — transparent to clicks, but children can still be hit.
  - `TETHER_HIT_IGNORE_ALL` — skip self AND all children entirely.

```c
/* Returns the topmost entity at screen position (x, y) */
Tether_GUID tether_hit_test(float x, float y);
```

**Step 3: State Machine**
The Input System maintains two stateful trackers: `hovered_entity` and `pressed_entity`. On each pointer event, it computes state transitions and fires the corresponding callbacks:

```
POINTER_MOVE:
  If new_hit != hovered_entity:
    → dispatch HOVER_EXIT on old entity
    → dispatch HOVER_ENTER on new entity
    → update hovered_entity

POINTER_DOWN:
  → dispatch PRESS on hovered_entity
  → set pressed_entity = hovered_entity

POINTER_UP:
  → dispatch RELEASE on pressed_entity
  If pointer is still over pressed_entity:
    → dispatch CLICK on pressed_entity
  → clear pressed_entity
```

**Step 4: Event Dispatch**
```c
typedef enum {
    TETHER_EVENT_HOVER_ENTER,
    TETHER_EVENT_HOVER_EXIT,
    TETHER_EVENT_PRESS,
    TETHER_EVENT_RELEASE,
    TETHER_EVENT_CLICK
} Tether_EventType;

typedef void (*Tether_EventCallback)(Tether_GUID entity,
                                      Tether_EventType type,
                                      void* user_data);

/* Bind a C function to an entity for a specific event */
void tether_bind_event(Tether_GUID entity, Tether_EventType type,
                        Tether_EventCallback callback, void* user_data);

/* Remove a binding */
void tether_unbind_event(Tether_GUID entity, Tether_EventType type);

/* Internal: fires the registered callback for an entity+event pair */
void tether_dispatch_event(Tether_GUID entity, Tether_EventType type);
```

---

### 2.4 The Hierarchy & Root Manager

#### Tree Manipulation API
```c
/* Attach an entity as the last child of a parent (updates linked list) */
void tether_tree_attach(Tether_GUID parent, Tether_GUID child);

/* Detach an entity from the tree (stitches siblings back together) */
void tether_tree_detach(Tether_GUID child);

/* Bring an entity to the front of its parent's local draw order */
void tether_tree_bring_to_front(Tether_GUID entity);
```

#### Root Management
The Engine (not the Kernel) manages the concept of "screens." A root entity is simply a regular entity whose `SlotTransform` matches the screen dimensions. The Engine maintains two roots:
- **Main Root:** The primary UI tree. All normal widgets live under this.
- **Overlay Root:** A second tree drawn on top of the Main Root. Used for tooltips, dropdowns, and modals that must visually float above everything.

```c
Tether_GUID tether_engine_get_main_root(void);
Tether_GUID tether_engine_get_overlay_root(void);
```

---

### 2.5 The Parser System

#### Purpose
The Parser system translates external descriptions into live ECS entities using the Widget Registry and Kernel APIs. It is a **consumer** of the Central Registry — when it reads `- Button:` in YAML, it calls `tether_widget_create("Button", parent)`. It does not know how to build a Button; it just asks the registry.

#### Supported Modes

| Mode | Description | Status |
|---|---|---|
| **YAML (Interpreted)** | Live parsing of `.yaml` files into ECS entities at runtime via `libyaml`. Supports hierarchy, properties, template variables (`$var`), and **Scene Composition** (`Scene: "path.yaml"` to embed YAMLs). Ideal for development and hot-reload. | ✅ Implemented |
| **TBC (Compiled / AOT)** | Pre-compiled binary format (`.tbc`). YAML is "cooked" offline into raw bytes by `tether_cooker.c`. Zero parsing overhead at runtime. Ideal for production/embedded/MCU. | 🔮 Future |
| **AST (JIT)** | The YAML parser produces an intermediate AST that supports variables, templates (widget definitions), and slot injection. Enables runtime composition and dynamic UI generation. | ✅ Implemented |

#### Core API
```c
/* Parse a YAML file and spawn entities as children of the given parent.
 * If parent is TETHER_INVALID_GUID, entities are attached to the main root. */
Tether_GUID tether_yaml_load(const char* filepath, Tether_GUID parent);
```

#### Key Files
```
src/parsers/
    tether_yaml.h             ← Parser API
    tether_yaml.c             ← YAML → ECS entity pipeline (uses libyaml)
    tether_yaml_ast.h/.c      ← AST intermediate representation
```

---

### 2.6 Engine Key Files (Target Structure)
```
include/tether/engine/
    tether_engine.h           ← Engine initialization + root management
    tether_registry.h         ← Central Registry API (ID, Layout, Widget)
    tether_layout.h           ← Layout system API
    tether_input.h            ← Input & hit testing API
    tether_events.h           ← Event binding & dispatch API
    tether_engine_components.h ← System Components (SlotTransform, Hierarchy, etc.)

src/engine/
    tether_engine.c           ← Engine init, root management
    tether_registry.c         ← Central Registry implementation
    tether_layout.c           ← Layout orchestration (2-pass traversal)
    tether_input.c            ← Hit testing + pointer state machine
    tether_events.c           ← Event callback storage + dispatch
```

---

## Layer 3: The UI Library (UMG)

#### Purpose
This is the layer where concrete UI primitives, aesthetics, and container rules are defined. It is analogous to **UMG in Unreal Engine**. Everything here is built entirely on top of Layers 1–2. A developer's custom widget has the exact same power and access as a Tether built-in widget.

---

### 3.1 Domain Components

The Engine handles the math, but the UI Library handles the **identity**. It defines the ECS components that give entities their visual appearance and behavior.

```c
/* --- Helper Types --- */
typedef struct { float x, y; } Tether_Vec2;
typedef struct { float top, right, bottom, left; } Tether_Edges;  /* CSS Order */
typedef struct { uint8_t r, g, b, a; } Tether_Color;

/* --- Visual Appearance --- */
typedef struct {
    Tether_Color  bg_color;
    Tether_Color  border_color;
    float         border_width;
    Tether_Edges  border_radius;   /* Per-corner radius for rounded rectangles */

    /* Interactive state colors (only take effect if entity has Interactable) */
    Tether_ColorMode hover_color_mode;  /* NONE, AUTO (±30 brightness), MANUAL */
    Tether_Color     hover_color;       /* Used when mode == MANUAL */
    Tether_ColorMode press_color_mode;
    Tether_Color     press_color;
    
    /* Retained Mode RHI Handle */
    void* render_handle;
} Tether_Style;

/* --- Text Rendering --- */
typedef enum { TETHER_FONT_NORMAL = 0, TETHER_FONT_BOLD, TETHER_FONT_ITALIC } Tether_FontStyle;

typedef struct {
    float             font_size;
    uint32_t          font_id;       /* From font registry */
    Tether_FontStyle  font_style;
    Tether_Align      align_x;       /* Text alignment within bounds */
    Tether_Align      align_y;
    float             wrap_width;    /* If > 0, wrap at this exact pixel width */
    
    /* Retained Mode RHI Handle */
    void* text_handle;
} Tether_Text;

/* --- Image Rendering --- */
typedef struct {
    char asset_path[128];  /* Path to PNG/JPG/SVG asset */
} Tether_Image;

/* --- Interaction --- */
typedef struct {
    uint8_t is_hovered;   /* Written by Input System */
    uint8_t is_pressed;   /* Written by Input System */
    uint8_t has_focus;    /* For future keyboard focus */
} Tether_Interactable;

/* --- Clipping --- */
typedef struct {
    uint8_t active;  /* If 1, children are clipped to this entity's bounds */
} Tether_ClipMask;
```

---

### 3.2 Container Rules (Slot Components)

Containers define how parents position their children. Each container type has its own **slot component** that children carry to tell the parent how to lay them out.

```c
/* --- Flex Container (Row / Column) ---
 * The FlexFlow component marks an entity as a flex container.
 * Its children carry FlexSlot to define how they participate in flex layout. */
typedef enum { TETHER_FLOW_NONE = 0, TETHER_FLOW_ROW, TETHER_FLOW_COLUMN } Tether_Flow;

typedef struct {
    Tether_Flow  flow;              /* ROW or COLUMN */
    Tether_Align content_align_x;   /* Cross-axis alignment of children */
    Tether_Align content_align_y;
    Tether_Edges padding;           /* Inner spacing from container edges */
    Tether_Vec2  gap;               /* Space between children */
} Tether_FlexFlow;

typedef struct {
    Tether_Edges margin;            /* Outer spacing from siblings */
    Tether_Vec2  explicit_size;     /* Fixed size (0 = auto/intrinsic) */
    float        fill_ratio;        /* 0.0 = use intrinsic, >0 = share remaining space */
    Tether_Align align_self_x;      /* Override parent's cross-axis alignment */
    Tether_Align align_self_y;
} Tether_FlexSlot;

/* --- Canvas Container (Absolute Positioning) ---
 * Children use AnchorSlot to define position relative to parent bounds. */
typedef struct {
    Tether_Vec2  anchor_min;  /* (0,0)=top-left, (1,1)=bottom-right */
    Tether_Vec2  anchor_max;
    Tether_Edges offset;      /* Pixel insets from anchored edges */
} Tether_AnchorSlot;
```

**At startup, Layer 3 registers these into the Engine's Layout Strategy Registry:**
```c
/* Container math */
tether_layout_register_strategy(COMP_FLEX_FLOW, measure_flex, arrange_flex);

/* Content math (Text, Image) */
tether_layout_register_strategy(COMP_TEXT_STYLE, measure_text, arrange_text);
tether_layout_register_strategy(COMP_IMAGE, measure_image, arrange_image);
```

---

### 3.3 Widgets (Factories)

A Widget is just a C function that creates an entity, attaches the right combination of System Components (Layer 2) and Domain Components (Layer 3), and registers it in the hierarchy.

```c
/* Example: Button = Panel + centered content + Interactable + child Text */
Tether_GUID tether_widget_create_button(Tether_GUID parent) {
    Tether_GUID entity = tether_widget_create_panel(parent);

    /* Layout: center children and add padding */
    Tether_FlexFlow* flow = tether_component_get(entity, COMP_FLEX_FLOW);
    flow->content_align_x = TETHER_ALIGN_CENTER;
    flow->content_align_y = TETHER_ALIGN_CENTER;
    flow->padding = (Tether_Edges){10, 20, 10, 20};

    /* Style: blue background with auto hover/press feedback */
    Tether_Style* s = tether_component_get(entity, COMP_STYLE);
    s->bg_color = (Tether_Color){97, 175, 239, 255};
    s->hover_color_mode = TETHER_COLOR_MODE_AUTO;
    s->press_color_mode = TETHER_COLOR_MODE_AUTO;

    /* Make it interactive */
    tether_component_add(entity, COMP_INTERACTABLE);

    /* Create a child Text widget with white text */
    Tether_GUID text = tether_widget_create_text(entity);
    tether_ecs_set_text_string(text, "Button");
    Tether_Style* ts = tether_component_get(text, COMP_STYLE);
    ts->bg_color = (Tether_Color){255, 255, 255, 255};

    return entity;
}
```

**Developer-Created Widgets (exact same API):**
```c
Tether_GUID create_health_bar(Tether_GUID parent) {
    Tether_GUID root = tether_widget_create_panel(parent);
    /* ... configure fill, colors, child elements ... */
    return root;
}

/* Register it — now YAML can use "HealthBar:" */
tether_widget_register("HealthBar", create_health_bar);
```

#### Built-in Widgets
| Widget | What it is in ECS terms |
|---|---|
| **Canvas** | Panel with `FlexFlow.flow = NONE` (children use AnchorSlot) |
| **Row** | Panel with `FlexFlow.flow = ROW` (children use FlexSlot) |
| **Column** | Panel with `FlexFlow.flow = COLUMN` (children use FlexSlot) |
| **Text** | Panel + `TextStyle` + text data component |
| **Button** | Panel + `Interactable` + centered `Text` child |
| **Image** | Panel + `Image` component (asset path) |

#### Key Files (Target Structure)
```
include/tether/ui/
    tether_widgets.h          ← Public widget creation API
    tether_ui_components.h    ← Domain Components (Style, TextStyle, Image, etc.)

src/ui/
    tether_widgets.c          ← Built-in widget factory implementations
    tether_layout_flex.c      ← Flex container measure/arrange logic
    tether_layout_canvas.c    ← Canvas container measure/arrange logic
    tether_content_text.c     ← Text content measure logic (calls RHI)
    tether_content_image.c    ← Image content measure logic
```

---

## The Big Picture: How It All Connects

### Full Initialization Flow
```
tether_run(config)
│
├── Layer 1: tether_kernel_init()
│   └── Allocate Sparse Sets, Dense Arrays, entity pools
│
├── Layer 2: Register System Components
│   ├── tether_component_register(sizeof(SlotTransform))   → ID 0
│   ├── tether_component_register(sizeof(RenderTransform))  → ID 1
│   ├── tether_component_register(sizeof(Hierarchy))        → ID 2
│   └── tether_component_register(sizeof(Visibility))       → ID 3
│
├── Layer 3: Register Domain Components
│   ├── tether_component_register(sizeof(Style))            → ID 4
│   ├── tether_component_register(sizeof(TextStyle))        → ID 5
│   ├── tether_component_register(sizeof(FlexFlow))         → ID 6
│   └── ... etc.
│
├── Layer 2: Initialize Registries + Roots
│   ├── tether_registry_init()
│   └── tether_engine_init_roots()   → create main_root + overlay_root entities
│
├── Layer 3: Register Widgets + Layout Strategies
│   ├── tether_widget_register("Canvas", create_canvas)
│   ├── tether_widget_register("Button", create_button)
│   ├── tether_layout_register_strategy(COMP_FLEX, measure_flex, arrange_flex)
│   └── tether_layout_register_strategy(COMP_TEXT, measure_text, arrange_text)
│
├── Layer 2: Parse YAML
│   └── tether_yaml_load(config->initial_yaml, TETHER_INVALID_GUID)
│       └── For each "Button:" → tether_widget_create("Button", parent)
│
├── User: config->on_init()
│   └── Developer binds events, registers custom widgets
│
└── Layer 1: tether_hal_run(config)
    └── Start the OS window loop
```

### Per-Frame Loop
```
┌──────────────────────────────────────────────────────┐
│ HAL polls OS events                                  │  Layer 1
│   └── Converts to Tether_Pointer_Event               │
├──────────────────────────────────────────────────────┤
│ Input System processes events                        │  Layer 2
│   ├── Hit test (reverse DFS with spatial culling)    │
│   ├── State machine (hover → press → release → click)│
│   └── Dispatch callbacks to developer functions      │
├──────────────────────────────────────────────────────┤
│ Layout System runs 2-pass traversal                  │  Layer 2
│   ├── Pass 1: Measure bottom-up (intrinsic sizes)    │
│   ├── Pass 2: Arrange top-down (distribute space)    │
│   └── Content re-measure (height-for-width)          │
├──────────────────────────────────────────────────────┤
│ RHI reads ECS arrays and draws                       │  Layer 1
│   ├── Walk Hierarchy for correct Z-order             │
│   ├── SlotTransform[] → positions                    │
│   ├── Style[] → backgrounds, borders, radii          │
│   ├── TextStyle[] + text data → glyphs               │
│   ├── Image[] → sprites                              │
│   └── Apply RenderTransform, ClipMask                │
├──────────────────────────────────────────────────────┤
│ HAL presents frame to display                        │  Layer 1
└──────────────────────────────────────────────────────┘
```

---

## What Needs to Change in the Codebase

The current codebase does not yet reflect this architecture. Here are the concrete refactoring tasks:

1. **Kernel Purity:** Move `Tether_Hierarchy` and `tether_ecs_find_by_id` out of `tether_ecs.c` and into Engine-layer files. Rename `src/core/` → `src/kernel/`.

2. **Component Separation:** Split `tether_components.h` into:
   - `tether_engine_components.h` (SlotTransform, RenderTransform, Hierarchy, Visibility)
   - `tether_ui_components.h` (Style, TextStyle, FlexFlow, FlexSlot, AnchorSlot, Image, Interactable, ClipMask)

3. **Endless Component Registration:** Replace `#define TETHER_COMPONENT_MAX 15` with dynamic `g_next_component_id` auto-increment system.

4. **Layout Strategy Registry:** Replace hardcoded `if (flow)` checks in `tether_layout.c` with the dynamic registry lookup pattern.

5. **ID Hash Map:** Replace the $O(N)$ string loop in `tether_ecs_find_by_id` with a Robin Hood Hash Map for $O(1)$ lookups.

6. **RHI Renaming:** Rename `tether_raster.h` → `tether_rhi.h`, `thorvg_raster.c` → `thorvg_rhi.c`.

7. **Directory Restructure:** Move files to match the target structure:
   - `src/core/` → `src/kernel/` (only `tether_ecs.c`)
   - `src/core/tether_events.c`, `tether_input.c`, `tether_registry.c` → `src/engine/`
   - `src/ui/tether_layout.c` → `src/engine/tether_layout.c`

## Current Limited

- Radius control per edge for rectshape (1 radius for all edge)
- Render translation scal x and y is not available (only support uniform scaler float)

---
