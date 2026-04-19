#pragma once

namespace ui_types {

    struct SpriteEntry {
        const char* id;
        const char* image_id;
        int x, y;
        bool visible;
    };

    struct TextEntry {
        const char* id;
        const char* text;
        int x, y;
        bool center_align;
        bool blink;
        int  blink_interval;
        bool visible;
    };

    struct ScreenData {
        const char* bg_image_id;
        int bg_scroll_x, bg_scroll_y;
        int sprite_count;
        const SpriteEntry* sprites;
        int text_count;
        const TextEntry* texts;
    };

} // namespace ui_types
