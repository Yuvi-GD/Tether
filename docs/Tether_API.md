# Tether: Complete API Reference

This document outlines the API for the Tether engine, strictly separated by our **3-Layer Tree Branch Architecture**. It includes the core data structures and function signatures that power the system.

---

## Layer 1: The Foundations (Kernel, HAL, RHI)

> **Philosophy:** The three base pillars. They know nothing about each other and know nothing about UI rules. 

### 1A. The Kernel (ECS Memory Allocator)
*Pure memory management. Operates entirely on numeric GUIDs and byte blocks.*

```c
/* 64-bit ID. Lower 32=Index, Upper 32=Generation Version */
typedef uint64_t Tether_GUID;
#define TETHER_INVALID_GUID 0

/* Maps an Entity Index to its position in a Dense Array */
typedef struct Tether_SparseMap {
    uint32_t* dense_indices;
    uint32_t capacity;
} Tether_SparseMap;

/* A perfectly contiguous block of component data */
typedef struct Tether_DenseArray {
    void* data;              /* Raw block of struct memory */
    Tether_GUID* entity_map; /* Maps dense index back to Entity GUID */
    size_t element_size;
    uint32_t count;
    uint32_t capacity;
} Tether_DenseArray;
```

### API: Entity Management
```c
void tether_kernel_init(void);
void tether_kernel_term(void);

Tether_GUID tether_entity_create(void);
void tether_entity_destroy(Tether_GUID entity);
bool tether_entity_is_valid(Tether_GUID entity);

/* Endless Dynamic Registration */
uint32_t tether_component_register(size_t element_size);

/* Memory Access */
void* tether_component_add(Tether_GUID entity, uint32_t component_id);
void* tether_component_get(Tether_GUID entity, uint32_t component_id); /* O(1) */
void  tether_component_remove(Tether_GUID entity, uint32_t component_id);

/* Exposes the raw contiguous array for the Engine systems to iterate over */
Tether_DenseArray* tether_component_get_array(uint32_t component_id);
```

### 1B. The HAL (Hardware Abstraction Layer)
*Manages the OS window and captures hardware events.*

```c
typedef enum {
    TETHER_MEMORY_HIGH_WATER_MARK = 0,
    TETHER_MEMORY_AGGRESSIVE_SHRINK
} Tether_Memory_Strategy;

typedef enum {
    TETHER_RENDERER_WEBGPU = 0,
    TETHER_RENDERER_SOFTWARE_CPU
} Tether_Renderer_Backend;

typedef struct {
    int width;
    int height;
    const char* title;
    const char* initial_yaml;
    
    Tether_Memory_Strategy memory_strategy;
    Tether_Renderer_Backend renderer;
    int target_fps;
    
    void (*on_init)(void);
} Tether_App_Config;

/* Start the OS windowing loop. Dispatches to Engine and RHI. */
void tether_hal_run(Tether_App_Config* config);
```

### 1C. The RHI (Render Hardware Interface)
*Formerly Rasterizer. Turns mathematical coordinates into pixels on screen.*

```c
void tether_rhi_init(uint32_t width, uint32_t height, const void* device, const void* instance);
void tether_rhi_resize(uint32_t width, uint32_t height);
void tether_rhi_draw(void);
const void* tether_rhi_get_texture(void);
void tether_rhi_term(void);

/* Retained Mode Granular Sync API */
void* tether_rhi_create_rect(void);
void* tether_rhi_create_text(void);
void* tether_rhi_create_scene(void);
void tether_rhi_scene_push(void* scene_handle, void* child_handle);
void tether_rhi_scene_remove(void* scene_handle, void* child_handle);
void tether_rhi_add_to_canvas(void* render_handle);

/* Setters */
void tether_rhi_set_rect_geometry(void* handle, float w, float h, float rx, float ry);
void tether_rhi_set_fill_color(void* handle, Tether_Color color);
void tether_rhi_set_text_string(void* handle, const char* str);
void tether_rhi_set_text_font(void* handle, uint32_t font_id, int style, float size);
void tether_rhi_translate(void* handle, float x, float y);

void tether_rhi_measure_text(const char* text, uint32_t font_id, int font_style, float font_size, float max_width, float* out_w, float* out_h);
```

---

## Layer 2: The Engine (Orchestration & Systems)

> **Philosophy:** The trunk of the tree. Bridges the foundations together. Defines the mandatory "System Components" required for math, traversal, and event handling.

### 1. The Central Registry
```c
/* --- ID Registry (String -> GUID) --- */
void tether_id_register(const char* string_id, Tether_GUID entity);
Tether_GUID tether_id_find(const char* string_id); /* O(1) Robin Hood Hash */

/* --- Layout Registry (Container-Agnostic Setup) --- */
typedef void (*Tether_MeasureFn)(Tether_GUID entity, void* container_data);
typedef void (*Tether_ArrangeFn)(Tether_GUID entity, void* container_data, float cx, float cy, float cw, float ch);
void tether_layout_register_strategy(uint32_t component_id, Tether_MeasureFn measure, Tether_ArrangeFn arrange);

/* --- Widget Registry (Factories) --- */
typedef Tether_GUID (*Tether_WidgetCreateFn)(Tether_GUID parent);
void tether_widget_register(const char* name, Tether_WidgetCreateFn factory_fn);
Tether_GUID tether_widget_create(const char* name, Tether_GUID parent);
```

### 2. The Hierarchy & Root Manager
```c
/* The mandatory structural component */
typedef struct {
    Tether_GUID parent;
    Tether_GUID first_child;
    Tether_GUID last_child;
    Tether_GUID prev_sibling;
    Tether_GUID next_sibling;
    uint32_t child_count;
} Tether_Hierarchy;

/* Tree manipulation API */
void tether_tree_attach(Tether_GUID parent, Tether_GUID child);

/* Detach an entity from the tree (stitches siblings together) */
void tether_tree_detach(Tether_GUID child);

/* Screen Management */
Tether_GUID tether_engine_get_main_root(void);
Tether_GUID tether_engine_get_overlay_root(void);

/* --- Rendering Engine (Retained Mode) --- */
/* The Render Engine maps ECS entities to RHI handles. */
void tether_render_init(void);
void tether_render_realize(Tether_GUID entity);
void tether_render_unrealize(Tether_GUID entity);
void tether_render_frame(float screen_w, float screen_h, bool force_redraw);
```

### 3. The Layout Engine
```c
/* The mathematical constraint components */
typedef struct { float x, y, width, height; } Tether_SlotTransform;
typedef struct { float tx, ty, sx, sy, rot, px, py; } Tether_RenderTransform;

/* Process the UI tree using the registered strategies */
void tether_layout_process(Tether_GUID root, float screen_width, float screen_height);
```

### 4. Input & Event System
```c
typedef enum { TETHER_POINTER_DOWN = 0, TETHER_POINTER_UP, TETHER_POINTER_MOVE } Tether_Pointer_EventType;
typedef enum { TETHER_MOUSE_BUTTON_LEFT = 0, TETHER_MOUSE_BUTTON_RIGHT, TETHER_MOUSE_BUTTON_NONE } Tether_MouseButton;

typedef struct {
    Tether_Pointer_EventType type;
    Tether_MouseButton button;
    float x;
    float y;
} Tether_Pointer_Event;

typedef enum { HOVER_ENTER, HOVER_EXIT, PRESS, RELEASE, CLICK } Tether_EventType;
typedef void (*Tether_EventCallback)(Tether_GUID entity, Tether_EventType type, void* user_data);

/* Input API */
Tether_GUID tether_hit_test(float x, float y);
void tether_input_process_event(Tether_Pointer_Event* event);

/* Event Dispatch API */
void tether_bind_event(Tether_GUID entity, Tether_EventType type, Tether_EventCallback callback, void* user_data);
```

---

## Layer 3: The UI Library (UMG)

> **Philosophy:** Concrete UI primitives built entirely using Layer 2 systems. Defines the Domain Components that give an entity its aesthetic identity.

### Domain Components & Container Rules (Examples)
```c
/* Aesthetics */
typedef struct { float top, right, bottom, left; } Tether_Edges;
typedef struct { float x, y; } Tether_Vec2;
typedef struct { uint8_t r, g, b, a; } Tether_Color;

/* Defines the absolute final bounds of a widget on screen */
typedef struct {
    float x, y, width, height;
} Tether_SlotTransform;

/* Defines visual aesthetics */
typedef struct {
    Tether_Color bg_color;
    Tether_Color border_color;
    float border_width;
    Tether_Edges border_radius;
    void* render_handle;
} Tether_Style;

/* Text constraints */
typedef struct {
    float font_size;
    uint32_t font_id;
    Tether_FontStyle font_style;
    Tether_Align align_x;
    Tether_Align align_y;
    float wrap_width;
    void* text_handle;
} Tether_Text;

/* Container Constraint */
typedef struct {
    Tether_Edges margin;
    Tether_Vec2 explicit_size;
    float fill_ratio; 
} Tether_FlexSlot;
```

### Built-in Widget Factories & Lifecycle
```c
Tether_GUID tether_widget_create_panel(Tether_GUID parent);
Tether_GUID tether_widget_create_text(Tether_GUID parent);
Tether_GUID tether_widget_create_button(Tether_GUID parent);

/* Realizes the widget on screen (creates GPU resources) */
void tether_widget_show(Tether_GUID entity);

/* Destroys the widget, its GPU resources, and all its children */
void tether_widget_destroy(Tether_GUID entity);
```

### Special ECS Tag Components (0-Size)
Tether uses 0-sized components (tags) to efficiently signal state without allocating memory:
- `TETHER_COMPONENT_DIRTY_LAYOUT`: Signals that the layout engine must remeasure this entity.
- `TETHER_COMPONENT_DIRTY_VISUAL`: Signals that the RHI must update this entity's styling/transform.
- `TETHER_COMPONENT_DIRTY_HIERARCHY`: Signals that children were added/removed.
- `TETHER_COMPONENT_VOLATILE`: Forces the RHI to fully recreate the entity's GPU resources every frame (used for elements bridging immediate mode and retained mode).

