#include "tether/ui/widgets/tether_text.h"
#include "tether/ui/tether_widgets.h"
#include <string.h>
#include <stdlib.h>

/* ========================================================================== */
/* 1. Widget Creation & Visual Sync                                           */
/* ========================================================================== */

Tether_GUID tether_ui_text(Tether_GUID parent) {
    Tether_GUID entity = tether_widget_create_container(parent);
    tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT);
    Tether_FlexSlot *f = (Tether_FlexSlot*)tether_ecs_get_component(entity, TETHER_COMPONENT_FLEX_SLOT);
    if (f) {
        f->fill_ratio = 0.0f;
    }
    return entity;
}

/* ========================================================================== */
/* 2. Operations (View Management & Activation)                               */
/* ========================================================================== */

const char* tether_ui_get_text(Tether_GUID entity) {
    if (!tether_ecs_is_valid(entity)) return NULL;

    Tether_TextWord* word = (Tether_TextWord*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_WORD);
    if (word) return word->data;

    Tether_TextLabel* label = (Tether_TextLabel*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_LABEL);
    if (label) return label->data;

    Tether_TextParagraph* paragraph = (Tether_TextParagraph*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_PARAGRAPH);
    if (paragraph) return paragraph->data;

    Tether_TextDynamic* dynamic = (Tether_TextDynamic*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_DYNAMIC);
    if (dynamic) return dynamic->data;

    return NULL;
}

bool tether_ui_set_text(Tether_GUID entity, const char* string) {
    if (!tether_ecs_is_valid(entity)) return false;
    if (!string) string = "";

    size_t length = strlen(string);
    Tether_TextDynamic* dynamic = (Tether_TextDynamic*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT_DYNAMIC);
    if (dynamic && dynamic->data) {
        free(dynamic->data);
        dynamic->data = NULL;
    }

    tether_ecs_remove_component(entity, TETHER_COMPONENT_TEXT_WORD);
    tether_ecs_remove_component(entity, TETHER_COMPONENT_TEXT_LABEL);
    tether_ecs_remove_component(entity, TETHER_COMPONENT_TEXT_PARAGRAPH);
    tether_ecs_remove_component(entity, TETHER_COMPONENT_TEXT_DYNAMIC);

    if (length <= 31) {
        Tether_TextWord* text = (Tether_TextWord*)tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT_WORD);
        if (!text) return false;
        memcpy(text->data, string, length + 1);
    } else if (length <= 127) {
        Tether_TextLabel* text = (Tether_TextLabel*)tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT_LABEL);
        if (!text) return false;
        memcpy(text->data, string, length + 1);
    } else if (length <= 511) {
        Tether_TextParagraph* text = (Tether_TextParagraph*)tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT_PARAGRAPH);
        if (!text) return false;
        memcpy(text->data, string, length + 1);
    } else {
        Tether_TextDynamic* text = (Tether_TextDynamic*)tether_ecs_add_component(entity, TETHER_COMPONENT_TEXT_DYNAMIC);
        if (!text) return false;
        text->capacity = (uint32_t)(length + 1) * 2;
        text->data = (char*)malloc(text->capacity);
        if (!text->data) {
            tether_ecs_remove_component(entity, TETHER_COMPONENT_TEXT_DYNAMIC);
            return false;
        }
        memcpy(text->data, string, length + 1);
        text->length = (uint32_t)length;
    }

    tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_TEXT_CONTENT);
    tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);

    return true;
}

void tether_ui_set_text_color(Tether_GUID entity, Tether_Color color) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Text *text = (Tether_Text*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT);
    if (text) {
        text->color = color;
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_text_size(Tether_GUID entity, float font_size) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Text *text = (Tether_Text*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT);
    if (text) {
        text->font_size = font_size;
        /* A new font size needs new glyph geometry, not just a new position. */
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_TEXT_CONTENT);
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}

void tether_ui_set_text_align(Tether_GUID entity, Tether_Align align_x, Tether_Align align_y) {
    if (entity == TETHER_INVALID_GUID) return;
    Tether_Text *text = (Tether_Text*)tether_ecs_get_component(entity, TETHER_COMPONENT_TEXT);
    if (text) {
        text->align_x = align_x;
        text->align_y = align_y;
        Tether_RenderTransform *transform =
            tether_ecs_get_component(entity, TETHER_COMPONENT_RENDER_TRANSFORM);
        if (transform && (transform->rotation_deg != 0.0f ||
                          transform->scale_x != 1.0f || transform->scale_y != 1.0f)) {
            tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_TEXT_CONTENT);
        }
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_LAYOUT);
        tether_ecs_add_component(entity, TETHER_COMPONENT_DIRTY_VISUAL);
    }
}
