#ifndef TETHER_COMPONENTS_H
#define TETHER_COMPONENTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Component ID Definitions
 */
#define TETHER_COMPONENT_SLOT_TRANSFORM   0
#define TETHER_COMPONENT_RENDER_TRANSFORM 1
#define TETHER_COMPONENT_STYLE            2
#define TETHER_COMPONENT_HIERARCHY        3
#define TETHER_COMPONENT_LAYOUT_NODE      4
#define TETHER_COMPONENT_ANCHOR_SLOT      5
#define TETHER_COMPONENT_FLEX_SLOT        6
#define TETHER_COMPONENT_TEXT_STYLE       7
#define TETHER_COMPONENT_TEXT_WORD        8
#define TETHER_COMPONENT_TEXT_LABEL       9
#define TETHER_COMPONENT_TEXT_PARAGRAPH   10
#define TETHER_COMPONENT_TEXT_DYNAMIC     11
#define TETHER_COMPONENT_CLIP_MASK        12
#define TETHER_COMPONENT_IMAGE            13
#define TETHER_COMPONENT_INTERACTABLE     14

/*
 * MATH VECTORS
 */
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
typedef enum { TETHER_ALIGN_FILL = 0, TETHER_ALIGN_LEFT, TETHER_ALIGN_CENTER, TETHER_ALIGN_RIGHT } Tether_AlignX;
typedef enum { TETHER_ALIGN_Y_FILL = 0, TETHER_ALIGN_TOP, TETHER_ALIGN_Y_CENTER, TETHER_ALIGN_BOTTOM } Tether_AlignY;

typedef enum {
    TETHER_FONT_NORMAL = 0,
    TETHER_FONT_BOLD = 1,
    TETHER_FONT_ITALIC = 2
} Tether_FontStyle;

typedef struct {
    float font_size;
    uint32_t font_id;
    Tether_FontStyle font_style;
    Tether_AlignX align_x;
    Tether_AlignY align_y;
} Tether_TextStyle;

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

#define TETHER_COMPONENT_MAX              15

/*
 * 1. THE OUTPUT (Calculated by the Engine)
 */
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

/*
 * 2. THE PARENT RULES (How I arrange my children)
 */
typedef enum { TETHER_FLOW_NONE = 0, TETHER_FLOW_ROW, TETHER_FLOW_COLUMN } Tether_Flow;

typedef struct Tether_LayoutNode {
    Tether_Flow flow;
    Tether_AlignX content_align_x;
    Tether_AlignY content_align_y;
    Tether_Edges padding;
} Tether_LayoutNode;

/*
 * 3. THE CHILD RULES (How I behave inside my parent)
 */

/* Used ONLY if Parent flow == NONE */
typedef struct Tether_AnchorSlot {
    Tether_Vec2 anchor_min;
    Tether_Vec2 anchor_max; 
    Tether_Edges offset;
} Tether_AnchorSlot;

/* Used ONLY if Parent flow == ROW/COLUMN */
typedef struct Tether_FlexSlot {
    Tether_Edges margin;
    float fill_ratio; 
    
    /* Self Overrides (optional) */
    uint8_t override_align_x; /* 1 = true, 0 = false */
    Tether_AlignX align_self_x;
    uint8_t override_align_y; 
    Tether_AlignY align_self_y;
} Tether_FlexSlot;

/*
 * 4. STYLING & VISUALS
 */
typedef struct Tether_Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Tether_Color;

typedef struct Tether_Style {
    Tether_Color bg_color;
    Tether_Color border_color;
    float border_width;
    Tether_Edges border_radius;
} Tether_Style;

/* Flag component: If present on entity, clips rendering of children to its bounds */
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
} Tether_Interactable;

typedef struct Tether_Hierarchy {
    uint64_t parent;
    uint64_t first_child;
    uint64_t last_child;
    uint64_t prev_sibling;
    uint64_t next_sibling;
    uint32_t child_count;
} Tether_Hierarchy;

#ifdef __cplusplus
}
#endif

#endif /* TETHER_COMPONENTS_H */
