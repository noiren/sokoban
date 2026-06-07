#include "generic_text_menu.h"

#include "input/input_manager.h"
#include "audio/sound_manager.h"

#include "bn_assert.h"
#include "bn_sprite_items_spr_selector_gold_bl.h"
#include "bn_sprite_items_spr_selector_gold_br.h"
#include "bn_sprite_items_spr_selector_gold_tl.h"
#include "bn_sprite_items_spr_selector_gold_tr.h"

namespace {

constexpr int kFramePadX = 6;
constexpr int kFramePadY = 2;
constexpr int kFrameWidth = 120;  // 左右コーナーの間隔（目視合わせ）
constexpr int kFrameHeight = 12;

} // namespace

GenericTextMenu::GenericTextMenu()
    : anchor_x_(-80), first_y_(-32), line_dy_(14), max_visible_(8) {
}

void GenericTextMenu::configure(bn::fixed anchor_x, bn::fixed first_y, bn::fixed line_dy, int max_visible) {
    anchor_x_ = anchor_x;
    first_y_  = first_y;
    line_dy_  = line_dy;
    max_visible_ = max_visible;
    if (max_visible_ < 1) {
        max_visible_ = 1;
    }
    if (max_visible_ > kMaxVisible) {
        max_visible_ = kMaxVisible;
    }
}

void GenericTextMenu::clear_items() {
    count_   = 0;
    cursor_  = 0;
    scroll_  = 0;
}

bool GenericTextMenu::push_item(int value, bn::string_view label) {
    if (count_ >= kMaxItems) {
        return false;
    }
    entries_[count_].value = value;
    entries_[count_].label.clear();
    // ラベル長すぎは切り詰め（UTF-8 境界は未考慮・短い日本語想定）
    const int max_copy = 44;
    int copied = 0;
    for (int i = 0; i < label.size() && copied < max_copy; ++i) {
        entries_[count_].label += label[i];
        ++copied;
    }
    ++count_;
    _clamp_cursor();
    _clamp_scroll();
    return true;
}

void GenericTextMenu::set_cursor(int index) {
    BN_ASSERT(count_ <= 0 || (index >= 0 && index < count_),
              "GenericTextMenu::set_cursor: index out of range");
    cursor_ = index;
    _clamp_cursor();
    _clamp_scroll();
}

void GenericTextMenu::_clamp_cursor() {
    if (count_ <= 0) {
        cursor_ = 0;
        return;
    }
    if (cursor_ < 0) {
        cursor_ = 0;
    }
    if (cursor_ >= count_) {
        cursor_ = count_ - 1;
    }
}

void GenericTextMenu::_clamp_scroll() {
    if (count_ <= 0) {
        scroll_ = 0;
        return;
    }
    if (count_ <= max_visible_) {
        scroll_ = 0;
        return;
    }
    if (cursor_ < scroll_) {
        scroll_ = cursor_;
    }
    if (cursor_ >= scroll_ + max_visible_) {
        scroll_ = cursor_ - max_visible_ + 1;
    }
    if (scroll_ < 0) {
        scroll_ = 0;
    }
    if (scroll_ > count_ - max_visible_) {
        scroll_ = count_ - max_visible_;
    }
}

GenericTextMenu::Poll GenericTextMenu::poll(int* out_value) {
    auto& inp = InputManager::instance();

    if (count_ <= 0) {
        if (inp.is_triggered(Action::Cancel)) {
            return Poll::Cancelled;
        }
        return Poll::None;
    }

    if (inp.is_triggered(Action::Cancel)) {
        return Poll::Cancelled;
    }

    if (inp.is_triggered(Action::Decide)) {
        if (out_value) {
            *out_value = entries_[cursor_].value;
        }
        return Poll::Confirmed;
    }

    bool moved = false;
    if (inp.is_repeat(Action::MoveUp)) {
        --cursor_;
        if (cursor_ < 0) {
            cursor_ = count_ - 1;
        }
        moved = true;
    }
    if (inp.is_repeat(Action::MoveDown)) {
        ++cursor_;
        if (cursor_ >= count_) {
            cursor_ = 0;
        }
        moved = true;
    }

    if (moved) {
        _clamp_scroll();
        SoundManager::instance().play_move();
        return Poll::Moved;
    }

    return Poll::None;
}

void GenericTextMenu::draw(bn::sprite_text_generator& gen, bn::vector<bn::sprite_ptr, 160>& out_sprites) {
    if (count_ <= 0) {
        gen.set_center_alignment();
        gen.generate(0, first_y_, "(項目なし)", out_sprites);
        return;
    }

    gen.set_left_alignment();

    const int show = count_ <= max_visible_ ? count_ : max_visible_;
    const bn::fixed box_left = anchor_x_ - bn::fixed(kFramePadX);
    const bn::fixed box_right = anchor_x_ + bn::fixed(kFrameWidth + kFramePadX);

    for (int r = 0; r < show; ++r) {
        const int i = scroll_ + r;
        BN_ASSERT(i >= 0 && i < count_, "generic menu row");

        bn::string<52> line;
        line.append(cursor_ == i ? ">" : " ");
        line.append(entries_[i].label);

        const bn::fixed y = first_y_ + line_dy_ * r;
        gen.generate(anchor_x_, y, line, out_sprites);

        if (cursor_ == i) {
            const bn::fixed fy_top = y - bn::fixed(kFramePadY);
            const bn::fixed fy_bot = y + bn::fixed(kFrameHeight);

            out_sprites.push_back(bn::sprite_items::spr_selector_gold_tl.create_sprite(box_left, fy_top));
            out_sprites.push_back(bn::sprite_items::spr_selector_gold_tr.create_sprite(box_right, fy_top));
            out_sprites.push_back(bn::sprite_items::spr_selector_gold_bl.create_sprite(box_left, fy_bot));
            out_sprites.push_back(bn::sprite_items::spr_selector_gold_br.create_sprite(box_right, fy_bot));
        }
    }
}
