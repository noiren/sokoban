#ifndef UI_TEXT_H
#define UI_TEXT_H

#include "ui_node.h"
#include "ui_types.h"
#include "bn_string.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"

class UIText : public UINode {
public:
    UIText(const bn::string_view& id, bn::fixed x, bn::fixed y, bool visible,
           ui_types::TextAlign align, int font_size,
           bool blink, int blink_interval, bn::sprite_text_generator& text_gen);

    void set_text(const bn::string_view& text);
    void set_align(ui_types::TextAlign align);
    void set_font_size(int size);

    // オーバーライド
    void set_visible(bool visible) override;
    void set_scale(bn::fixed scale) override; // アニメーション用フック
    void update() override;

private:
    void _rebuild();

    bn::string<64> text_;
    ui_types::TextAlign align_;
    int font_size_;
    bool blink_;
    int blink_interval_;
    int blink_timer_;
    bool blink_state_;
    bool dirty_;

    bn::vector<bn::sprite_ptr, 16> sprites_;
    bn::sprite_text_generator& text_gen_;
};

#endif // UI_TEXT_H