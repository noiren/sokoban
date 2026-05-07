#include "ui_text.h"

UIText::UIText(const bn::string_view& id, bn::fixed x, bn::fixed y, bool visible,
               ui_types::TextAlign align, int font_size,
               bool blink, int blink_interval, bn::sprite_text_generator& text_gen)
    : UINode(id, x, y, visible),
      align_(align), font_size_(font_size > 0 ? font_size : 1),
      blink_(blink), blink_interval_(blink_interval), blink_timer_(blink_interval),
      blink_state_(true), dirty_(true), text_gen_(text_gen) {}

void UIText::set_text(const bn::string_view& text) {
    if (text_ != text) {
        text_ = text;
        dirty_ = true;
    }
}

void UIText::set_align(ui_types::TextAlign align) {
    if (align_ != align) {
        align_ = align;
        dirty_ = true;
    }
}

void UIText::set_font_size(int size) {
    if (size > 0 && font_size_ != size) {
        font_size_ = size;
        dirty_ = true;
    }
}

void UIText::set_visible(bool visible) {
    if (visible_ != visible) {
        UINode::set_visible(visible);
        if (!visible_) {
            for (auto& sp : sprites_) sp.set_visible(false);
        } else {
            dirty_ = true;
        }
    }
}

void UIText::set_scale(bn::fixed scale) {
    // スケール値をフォントサイズにハックして割り当てる
    int new_size = scale.right_shift_integer();
    if (new_size < 1) new_size = 1;
    set_font_size(new_size);
}

void UIText::_rebuild() {
    sprites_.clear();
    if (!text_.empty()) {
        switch (align_) {
            case ui_types::TextAlign::LEFT:
                text_gen_.set_left_alignment();
                break;
            case ui_types::TextAlign::RIGHT:
                text_gen_.set_right_alignment();
                break;
            case ui_types::TextAlign::CENTER:
            default:
                text_gen_.set_center_alignment();
                break;
        }
        text_gen_.generate(x_, y_, text_, sprites_);
    }
}

void UIText::update() {
    if (!visible_) return;

    if (dirty_) {
        _rebuild();
        dirty_ = false;
        blink_timer_ = blink_interval_;
        blink_state_ = true;
    }

    if (blink_ && blink_interval_ > 0 && !sprites_.empty()) {
        blink_timer_--;
        if (blink_timer_ <= 0) {
            blink_timer_ = blink_interval_;
            blink_state_ = !blink_state_;
            for (auto& sp : sprites_) {
                sp.set_visible(blink_state_);
            }
        }
    }
}