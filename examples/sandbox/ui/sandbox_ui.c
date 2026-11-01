#include "sandbox_ui.h"
#include "tether/ui/tether_widgets.h"

Tether_GUID sandbox_ui_section(Tether_GUID parent, const char *title) {
    Tether_GUID col = tether_ui_column(parent, 6.0f);
    Tether_GUID lbl = tether_ui_text(col, title, 16.0f, (Tether_Color){145, 190, 225, 255});
    (void)lbl;
    return col;
}

Tether_GUID sandbox_ui_header(Tether_GUID parent, const char *title, const char *subtitle) {
    Tether_GUID col = tether_ui_column(parent, 4.0f);
    tether_ui_text(col, title, 24.0f, SANDBOX_COLOR_HEADER_TEXT);
    if (subtitle && subtitle[0]) {
        tether_ui_text(col, subtitle, 13.5f, SANDBOX_COLOR_SUB_TEXT);
    }
    return col;
}

Tether_GUID sandbox_ui_console_box(Tether_GUID parent, const char *tag_title, float wrap_width, Tether_GUID *out_text_entity) {
    Tether_GUID box = tether_ui_column(parent, 6.0f);
    tether_ui_set_padding(box, 10.0f, 14.0f, 10.0f, 14.0f);
    tether_ui_set_color(box, SANDBOX_COLOR_CONSOLE_BG);
    tether_ui_set_border(box, 1.0f, SANDBOX_COLOR_CONSOLE_BD, 7.0f);
    tether_ui_set_flex(box, 1.0f);

    if (tag_title && tag_title[0]) {
        tether_ui_text(box, tag_title, 11.0f, (Tether_Color){120, 150, 190, 255});
    }

    Tether_GUID text_id = tether_ui_text(box, "", 13.0f, (Tether_Color){175, 200, 225, 255});
    Tether_Text *t = (Tether_Text*)tether_ecs_get_component(text_id, TETHER_COMPONENT_TEXT);
    if (t) {
        t->wrap_width = wrap_width > 0.0f ? wrap_width : 540.0f;
    }

    if (out_text_entity) {
        *out_text_entity = text_id;
    }
    return box;
}

Tether_GUID sandbox_ui_button(Tether_GUID parent, const char *caption, Tether_EventCallback callback, void *user_data) {
    Tether_GUID panel = tether_widget_create_panel(parent);
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(panel, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->size_box.y = 48.0f;
    }
    Tether_FlexSlot *slot = (Tether_FlexSlot*)tether_ecs_get_component(panel, TETHER_COMPONENT_FLEX_SLOT);
    if (slot) {
        slot->fill_ratio = 1.0f;
    }

    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(panel, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->bg_color = (Tether_Color){39, 53, 74, 255};
        shape->border_radius = (Tether_Edges){7.0f, 7.0f, 7.0f, 7.0f};
    }

    Tether_Interactable *input = (Tether_Interactable*)tether_ecs_get_component(panel, TETHER_COMPONENT_INTERACTABLE);
    if (input) {
        input->hover_color_mode = TETHER_COLOR_MODE_AUTO;
        input->press_color_mode = TETHER_COLOR_MODE_AUTO;
    }

    Tether_GUID label = tether_widget_create_text(panel);
    Tether_Text *style = (Tether_Text*)tether_ecs_get_component(label, TETHER_COMPONENT_TEXT);
    if (style) {
        style->font_size = 17.0f;
        style->color = (Tether_Color){222, 232, 247, 255};
        style->align_x = TETHER_ALIGN_CENTER;
        style->align_y = TETHER_ALIGN_CENTER;
    }
    tether_ui_set_text(label, caption);

    Tether_FlexSlot *label_slot = (Tether_FlexSlot*)tether_ecs_get_component(label, TETHER_COMPONENT_FLEX_SLOT);
    if (label_slot) {
        label_slot->fill_ratio = 1.0f;
    }

    if (callback) {
        tether_bind_event(panel, TETHER_EVENT_CLICK, callback, user_data);
    }
    return label;
}

Tether_GUID sandbox_ui_line(Tether_GUID parent, float thickness, Tether_Color color) {
    Tether_GUID line = tether_widget_create_panel(parent);
    Tether_Layout *layout = (Tether_Layout*)tether_ecs_get_component(line, TETHER_COMPONENT_LAYOUT);
    if (layout) {
        layout->size_box.y = thickness;
        layout->size_box.x = 0.0f; /* Stretch horizontally */
    }
    Tether_Shape *shape = (Tether_Shape*)tether_ecs_get_component(line, TETHER_COMPONENT_SHAPE);
    if (shape) {
        shape->bg_color = color;
        shape->border_radius = (Tether_Edges){0.0f, 0.0f, 0.0f, 0.0f};
    }
    return line;
}