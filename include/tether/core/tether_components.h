#ifndef TETHER_COMPONENTS_H
#define TETHER_COMPONENTS_H

#include <stdint.h>
#include <stdbool.h>

#define TETHER_INVALID_HANDLE 0xFFFFFFFFU

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. COMPONENT ID DEFINITIONS                                                */
/* ========================================================================== */

/* --- CORE ENGINE COMPONENTS (Layer 1/2) --- */
enum {
    TETHER_COMPONENT_SLOT_TRANSFORM = 0,
    TETHER_COMPONENT_RENDER_TRANSFORM,
    TETHER_COMPONENT_HIERARCHY,
    TETHER_COMPONENT_VISIBILITY,
    TETHER_COMPONENT_OVERFLOW,
    TETHER_COMPONENT_INTERACTABLE,
    TETHER_COMPONENT_LAYOUT,
    TETHER_COMPONENT_ANCHOR_SLOT,
    TETHER_COMPONENT_FLEX_SLOT,

    TETHER_CORE_COMPONENTS_MAX
};

/* --- SYSTEM UI COMPONENTS (Layer 3) --- */
enum {
    /* State Components */
    TETHER_COMPONENT_DIRTY_LAYOUT = TETHER_CORE_COMPONENTS_MAX,
    TETHER_COMPONENT_DIRTY_VISUAL,
    TETHER_COMPONENT_DIRTY_HIERARCHY,
    TETHER_COMPONENT_DIRTY_TEXT_CONTENT,
    TETHER_COMPONENT_VOLATILE,
    TETHER_COMPONENT_IS_LEAF,

    /* Visual Components */
    TETHER_COMPONENT_SHAPE,
    TETHER_COMPONENT_IMAGE,
    TETHER_COMPONENT_TEXT,

    TETHER_COMPONENT_TEXT_WORD,
    TETHER_COMPONENT_TEXT_LABEL,
    TETHER_COMPONENT_TEXT_PARAGRAPH,
    TETHER_COMPONENT_TEXT_DYNAMIC,

    /* Semantic UI State Components (Pure ECS) */
    TETHER_COMPONENT_VALUE_RANGE,
    TETHER_COMPONENT_BOOL_STATE,
    TETHER_COMPONENT_SELECTION_STATE,

    TETHER_SYSTEM_COMPONENTS_MAX
};

/* Default boundary when an application does not define user components. */
#define TETHER_COMPONENT_MAX              TETHER_SYSTEM_COMPONENTS_MAX

/* ========================================================================== */
/* 2. ENUMS                                                                   */
/* ========================================================================== */
typedef enum { 
    TETHER_ALIGN_AUTO = 0,
    TETHER_ALIGN_FILL, 
    TETHER_ALIGN_START,   /* Left / Top */
    TETHER_ALIGN_CENTER, 
    TETHER_ALIGN_END      /* Right / Bottom */
} Tether_Align;

typedef enum {
    TETHER_FONT_NORMAL = 0,
    TETHER_FONT_BOLD,
    TETHER_FONT_ITALIC
} Tether_FontStyle;

typedef enum { 
    TETHER_FLOW_NONE = 0, 
    TETHER_FLOW_ROW, 
    TETHER_FLOW_COLUMN
} Tether_Flow;

typedef enum {
    TETHER_POSITION_RELATIVE = 0,
    TETHER_POSITION_ABSOLUTE,
    TETHER_POSITION_FIXED
} Tether_PositionMode;

typedef enum {
    TETHER_HIT_BLOCK = 0,       /* Catches click. Stops propagation to things behind. */
    TETHER_HIT_IGNORE,      /* Ghost: Clicks pass right through me AND my children. */
    TETHER_HIT_SELF_ONLY,   /* I catch clicks on my background, but clicks on children pass through. */
    TETHER_HIT_CHILD_ONLY   /* I cannot be clicked, but my interactive children CAN be. */
} Tether_HitBehavior;

typedef enum {
    TETHER_VISIBLE = 0,
    TETHER_HIDDEN,
    TETHER_COLLAPSED
} Tether_VisibilityState;

typedef enum {
    TETHER_OVERFLOW_CLIP = 0,
    TETHER_OVERFLOW_VISIBLE,
    TETHER_OVERFLOW_WRAP,
    TETHER_OVERFLOW_SCROLL
} Tether_OverflowMode;

/* ========================================================================== */
/* 3. HELPER STRUCTS (Not Final ECS Components)                               */
/* ========================================================================== */
typedef struct {
    float x;
    float y;
} Tether_Vec2;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Tether_Vec4;

/* CSS Standard Order: Top, Right, Bottom, Left */
typedef struct {
    float top;
    float right;
    float bottom;
    float left;
} Tether_Edges;

typedef struct Tether_Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Tether_Color;

/* ========================================================================== */
/* 4. ECS NATIVE COMPONENTS                                                   */
/* ========================================================================== */
typedef struct Tether_SlotTransform {
    float x;
    float y;
    float width;
    float height;
} Tether_SlotTransform;

#define TETHER_DEFAULT_SLOT_TRANSFORM (&(Tether_SlotTransform){ 0.0f, 0.0f, 0.0f, 0.0f })

/* Text rotation, scale and pivot changes require DIRTY_TEXT_CONTENT and DIRTY_VISUAL. */
typedef struct Tether_RenderTransform {
    float translation_x;
    float translation_y;
    float scale_x;
    float scale_y;
    float rotation_deg;
    Tether_Vec2 render_pivot; /* Default {0.5f, 0.5f} */
    float opacity; /* Default 1.0 */
} Tether_RenderTransform;

#define TETHER_DEFAULT_RENDER_TRANSFORM (&(Tether_RenderTransform){ \
    .translation_x = 0.0f, .translation_y = 0.0f, \
    .scale_x = 1.0f, .scale_y = 1.0f, \
    .rotation_deg = 0.0f, \
    .render_pivot = {0.5f, 0.5f}, \
    .opacity = 1.0f \
})

typedef struct Tether_Hierarchy {
    uint64_t parent;
    uint64_t first_child;
    uint64_t last_child;
    uint64_t prev_sibling;
    uint64_t next_sibling;
    uint32_t child_count;
} Tether_Hierarchy;

typedef struct {
    Tether_VisibilityState local_state;
    Tether_VisibilityState computed_state;
} Tether_Visibility;

#define TETHER_DEFAULT_VISIBILITY (&(Tether_Visibility){ \
    .local_state = TETHER_VISIBLE, \
    .computed_state = TETHER_VISIBLE \
})

typedef struct {
    Tether_Flow flow;          /* COLUMN, ROW, or NONE */
    Tether_PositionMode position; /* RELATIVE, ABSOLUTE, FIXED */
    Tether_Align content_align_x;
    Tether_Align content_align_y;
    Tether_Edges padding;
    Tether_Vec2 gap;
    Tether_Vec2 size_box;      /* 0,0 means auto/intrinsic */
    uint8_t wrap;
    float measured_width;
    float measured_height;
} Tether_Layout;

#define TETHER_DEFAULT_LAYOUT (&(Tether_Layout){ \
    .flow = TETHER_FLOW_NONE, \
    .position = TETHER_POSITION_RELATIVE, \
    .content_align_x = TETHER_ALIGN_FILL, \
    .content_align_y = TETHER_ALIGN_FILL, \
    .padding = {0.0f, 0.0f, 0.0f, 0.0f}, \
    .gap = {0.0f, 0.0f}, \
    .size_box = {0.0f, 0.0f}, \
    .wrap = 0, \
    .measured_width = 0.0f, \
    .measured_height = 0.0f \
})

typedef struct Tether_AnchorSlot {
    Tether_Vec2 anchor_min;
    Tether_Vec2 anchor_max; 
    Tether_Edges offset;
    Tether_Vec2 pivot;
} Tether_AnchorSlot;

#define TETHER_DEFAULT_ANCHOR_SLOT (&(Tether_AnchorSlot){ \
    .anchor_min = {0.0f, 0.0f}, \
    .anchor_max = {1.0f, 1.0f}, \
    .offset = {0.0f, 0.0f, 0.0f, 0.0f}, \
    .pivot = {0.0f, 0.0f} \
})

typedef struct Tether_FlexSlot {
    Tether_Edges margin;
    float fill_ratio; 
    
    /* Self Overrides (optional) */
    Tether_Align align_self_x;
    Tether_Align align_self_y;
} Tether_FlexSlot;

#define TETHER_DEFAULT_FLEX_SLOT (&(Tether_FlexSlot){ \
    .margin = {0.0f, 0.0f, 0.0f, 0.0f}, \
    .fill_ratio = 0.0f, \
    .align_self_x = TETHER_ALIGN_AUTO, \
    .align_self_y = TETHER_ALIGN_AUTO \
})

/* ========================================================================== */
/* 4. SYSTEM ECS COMPONENTS                                                   */
/* ========================================================================== */

typedef struct Tether_Shape {
    uint32_t handle;
    Tether_Color bg_color;
    Tether_Color border_color;
    Tether_Color active_bg;
    Tether_Color active_border;
    float border_width;
    Tether_Edges border_radius;
} Tether_Shape;

#define TETHER_DEFAULT_SHAPE (&(Tether_Shape){ \
    .handle = TETHER_INVALID_HANDLE, \
    .bg_color = {0, 0, 0, 0}, \
    .border_color = {0, 0, 0, 0}, \
    .active_bg = {0, 0, 0, 0}, \
    .active_border = {0, 0, 0, 0}, \
    .border_width = 0.0f, \
    .border_radius = {0.0f, 0.0f, 0.0f, 0.0f} \
})

typedef struct {
    uint32_t handle;
    float font_size;
    uint32_t font_id; /* Default 0 or TETHER_INVALID_FONT_ID invalid */
    Tether_FontStyle font_style;
    Tether_Align align_x;
    Tether_Align align_y;
    float wrap_width; /* If > 0, text wraps at this exact width */
    Tether_Color color;
    
    /* Computed state (updated by Text Measure System) */
    Tether_Vec2 intrinsic_size;
} Tether_Text;

#define TETHER_DEFAULT_TEXT (&(Tether_Text){ \
    .handle = TETHER_INVALID_HANDLE, \
    .font_size = 16.0f, \
    .font_id = 0, \
    .font_style = TETHER_FONT_NORMAL, \
    .align_x = TETHER_ALIGN_START, \
    .align_y = TETHER_ALIGN_START, \
    .wrap_width = 0.0f, \
    .color = {255, 255, 255, 255}, \
    .intrinsic_size = {0.0f, 0.0f} \
})

typedef struct {
    char data[32];
} Tether_TextWord;

typedef struct {
    char data[128];
} Tether_TextLabel;

typedef struct {
    char data[512];
} Tether_TextParagraph;

typedef struct {
    char* data;
    uint32_t length;
    uint32_t capacity;
} Tether_TextDynamic;

typedef struct Tether_Image {
    char asset_path[128];
} Tether_Image;

typedef struct Tether_Interactable {
    bool is_enabled;
    bool is_hovered;
    bool is_pressed;
    bool has_focus;
    Tether_HitBehavior hit_behavior;
} Tether_Interactable;

#define TETHER_DEFAULT_INTERACTABLE (&(Tether_Interactable){ \
    .is_enabled = true, \
    .is_hovered = false, \
    .is_pressed = false, \
    .has_focus = false, \
    .hit_behavior = TETHER_HIT_BLOCK \
})

typedef struct Tether_Overflow {
    Tether_OverflowMode x;
    Tether_OverflowMode y;
} Tether_Overflow;

#define TETHER_DEFAULT_OVERFLOW (&(Tether_Overflow){ \
    .x = TETHER_OVERFLOW_CLIP, \
    .y = TETHER_OVERFLOW_CLIP \
})

/* ========================================================================== */
/* Semantic UI State Components (Pure ECS)                                     */
/* ========================================================================== */

/* Value Range: 16 bytes (4 x 4-byte floats, SIMD-aligned) */
typedef struct {
    float value;
    float min_val;
    float max_val;
    float step;
} Tether_ValueRange;

#define TETHER_DEFAULT_VALUE_RANGE (&(Tether_ValueRange){ \
    .value = 0.0f, \
    .min_val = 0.0f, \
    .max_val = 1.0f, \
    .step = 0.0f \
})

/* Boolean State: 1 byte */
typedef struct {
    bool value;
} Tether_BoolState;

#define TETHER_DEFAULT_BOOL_STATE (&(Tether_BoolState){ \
    .value = false \
})

/* Selection State: 12 bytes */
typedef struct {
    int32_t selected_index;
    int32_t count;
    bool is_open;
} Tether_SelectionState;

#define TETHER_DEFAULT_SELECTION_STATE (&(Tether_SelectionState){ \
    .selected_index = -1, \
    .count = 0, \
    .is_open = false \
})

#ifdef __cplusplus
}
#endif

#endif /* TETHER_COMPONENTS_H */
