#ifndef TETHER_COMPONENTS_H
#define TETHER_COMPONENTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. COMPONENT ID DEFINITIONS                                                */
/* ========================================================================== */
/* --- CORE ENGINE COMPONENTS (Layer 1/2) --- */
#define TETHER_COMPONENT_SLOT_TRANSFORM   0
#define TETHER_COMPONENT_RENDER_TRANSFORM 1
#define TETHER_COMPONENT_HIERARCHY        2
#define TETHER_COMPONENT_VISIBILITY       3
#define TETHER_COMPONENT_LAYOUT_NODE      4
#define TETHER_CORE_COMPONENTS_MAX        5

/* --- SYSTEM UI COMPONENTS (Layer 3) --- */
#define TETHER_COMPONENT_STYLE            (TETHER_CORE_COMPONENTS_MAX + 0)
#define TETHER_COMPONENT_ANCHOR_SLOT      (TETHER_CORE_COMPONENTS_MAX + 1)
#define TETHER_COMPONENT_FLEX_SLOT        (TETHER_CORE_COMPONENTS_MAX + 2)
#define TETHER_COMPONENT_TEXT_STYLE       (TETHER_CORE_COMPONENTS_MAX + 3)
#define TETHER_COMPONENT_TEXT_WORD        (TETHER_CORE_COMPONENTS_MAX + 4)
#define TETHER_COMPONENT_TEXT_LABEL       (TETHER_CORE_COMPONENTS_MAX + 5)
#define TETHER_COMPONENT_TEXT_PARAGRAPH   (TETHER_CORE_COMPONENTS_MAX + 6)
#define TETHER_COMPONENT_TEXT_DYNAMIC     (TETHER_CORE_COMPONENTS_MAX + 7)
#define TETHER_COMPONENT_CLIP_MASK        (TETHER_CORE_COMPONENTS_MAX + 8)
#define TETHER_COMPONENT_IMAGE            (TETHER_CORE_COMPONENTS_MAX + 9)
#define TETHER_COMPONENT_INTERACTABLE     (TETHER_CORE_COMPONENTS_MAX + 10)

#define TETHER_SYSTEM_COMPONENTS_MAX      (TETHER_CORE_COMPONENTS_MAX + 11)

/* --- USER HARDCODED COMPONENTS --- */
/* Users can hardcode their components starting from TETHER_SYSTEM_COMPONENTS_MAX */
#define TETHER_USER_COMPONENTS_MAX        (TETHER_SYSTEM_COMPONENTS_MAX + 0)

/* --- DYNAMIC COMPONENTS --- */
/* Any components registered at runtime via tether_component_register() start here */
#define TETHER_COMPONENT_MAX              TETHER_USER_COMPONENTS_MAX

/* ========================================================================== */
/* 2. ENUMS                                                                   */
/* ========================================================================== */
typedef enum { 
    TETHER_ALIGN_FILL = 0, 
    TETHER_ALIGN_START,   /* Left / Top */
    TETHER_ALIGN_CENTER, 
    TETHER_ALIGN_END      /* Right / Bottom */
} Tether_Align;

typedef enum {
    TETHER_FONT_NORMAL = 0,
    TETHER_FONT_BOLD = 1,
    TETHER_FONT_ITALIC = 2
} Tether_FontStyle;

typedef enum { TETHER_FLOW_NONE = 0, TETHER_FLOW_ROW, TETHER_FLOW_COLUMN } Tether_Flow;

typedef enum {
    TETHER_HIT_BLOCK = 0,       /* Catches click. Stops propagation to things behind. */
    TETHER_HIT_IGNORE = 1,      /* Ghost: Clicks pass right through me AND my children. */
    TETHER_HIT_SELF_ONLY = 2,   /* I catch clicks on my background, but clicks on children pass through. */
    TETHER_HIT_CHILD_ONLY = 3   /* I cannot be clicked, but my interactive children CAN be. */
} Tether_HitBehavior;

typedef enum {
    TETHER_VISIBLE = 0,
    TETHER_HIDDEN,
    TETHER_COLLAPSED
} Tether_VisibilityState;

typedef enum {
    TETHER_COLOR_MODE_NONE   = 0, /* No state color — bg_color used always */
    TETHER_COLOR_MODE_AUTO   = 1, /* Derive lighter/darker from bg_color automatically */
    TETHER_COLOR_MODE_MANUAL = 2  /* Use explicit hover_color / press_color */
} Tether_ColorMode;

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
/* 4. ECS COMPONENTS                                                          */
/* ========================================================================== */
typedef struct Tether_SlotTransform {
    float x;
    float y;
    float width;
    float height;
} Tether_SlotTransform;

typedef struct Tether_RenderTransform {
    float translation_x;
    float translation_y;
    float scale_x;
    float scale_y;
    float rotation_deg;
    float pivot_x; /* Default 0.5 */
    float pivot_y; /* Default 0.5 */
} Tether_RenderTransform;

typedef struct Tether_Style {
    Tether_Color bg_color;
    Tether_Color border_color;
    float border_width;
    Tether_Edges border_radius;
} Tether_Style;

typedef struct Tether_Hierarchy {
    uint64_t parent;
    uint64_t first_child;
    uint64_t last_child;
    uint64_t prev_sibling;
    uint64_t next_sibling;
    uint32_t child_count;
} Tether_Hierarchy;

typedef struct {
    char id[32];               /* Human-readable ID for finding from C */
    Tether_Flow flow;          /* COLUMN, ROW, or NONE */
    Tether_Align content_align_x;
    Tether_Align content_align_y;
    Tether_Edges padding;
    Tether_Vec2 gap;
    uint8_t wrap;
    float measured_width;
    float measured_height;
} Tether_LayoutNode;

typedef struct Tether_AnchorSlot {
    Tether_Vec2 anchor_min;
    Tether_Vec2 anchor_max; 
    Tether_Edges offset;
} Tether_AnchorSlot;

typedef struct Tether_FlexSlot {
    Tether_Edges margin;
    Tether_Vec2 explicit_size; /* 0,0 means auto/intrinsic */
    float fill_ratio; 
    
    /* Self Overrides (optional) */
    uint8_t override_align_x; /* 1 = true, 0 = false */
    Tether_Align align_self_x;
    uint8_t override_align_y; 
    Tether_Align align_self_y;
} Tether_FlexSlot;

typedef struct {
    float font_size;
    uint32_t font_id;
    Tether_FontStyle font_style;
    Tether_Align align_x;
    Tether_Align align_y;
    float wrap_width; /* If > 0, text wraps at this exact width */
} Tether_TextStyle;

typedef struct {
    Tether_VisibilityState state;
} Tether_Visibility;

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

typedef struct Tether_ClipMask {
    uint8_t active;
} Tether_ClipMask;

typedef struct Tether_Image {
    char asset_path[128];
} Tether_Image;

typedef struct Tether_Interactable {
    uint8_t is_hovered;
    uint8_t is_pressed;
    uint8_t has_focus;
    
    Tether_HitBehavior hit_behavior;
    
    Tether_ColorMode hover_color_mode;
    Tether_Color hover_color;
    Tether_ColorMode press_color_mode;
    Tether_Color press_color;
} Tether_Interactable;

#ifdef __cplusplus
}
#endif

#endif /* TETHER_COMPONENTS_H */
