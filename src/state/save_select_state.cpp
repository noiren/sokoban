#include "save_select_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_palettes.h"

SaveSelectState::SaveSelectState(bn::sprite_text_generator& text_gen, SaveData& save)
    : text_gen_(text_gen), save_(save), cursor_(0), selected_slot_(-1) {
}

void SaveSelectState::init(StateManager& /*manager*/) {
    cursor_        = 0;
    selected_slot_ = -1;
    sprites_.clear();

    bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
    bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 0);

    draw_slots();
}

void SaveSelectState::update(StateManager& manager) {
    bool changed = false;

    if (bn::keypad::up_pressed()) {
        cursor_--;
        if (cursor_ < 0) cursor_ = NUM_SAVE_SLOTS - 1;
        changed = true;
    }
    if (bn::keypad::down_pressed()) {
        cursor_++;
        if (cursor_ >= NUM_SAVE_SLOTS) cursor_ = 0;
        changed = true;
    }

    if (changed) {
        draw_slots();
    }

    if (bn::keypad::a_pressed()) {
        selected_slot_ = cursor_;
        manager.pop();  // → main.cpp がスロットを受け取り MenuState へ
    }
}

void SaveSelectState::draw_slots() {
    sprites_.clear();
    text_gen_.set_center_alignment();

    // タイトル
    text_gen_.generate(0, -56, "セーブデータを選んでください", sprites_);

    for (int i = 0; i < NUM_SAVE_SLOTS; i++) {
        const SaveSlot& slot = save_.slots[i];
        bool valid = save_slot_is_valid(slot);

        // スロット番号 (y位置: -24, 0, 24)
        int y = -16 + i * 28;

        // スロット情報文字列を構築
        bn::string<48> line;
        if (i == cursor_) {
            line.append("> ");
        } else {
            line.append("  ");
        }

        line.append("SLOT ");
        line.append(bn::to_string<8>(i + 1));
        line.append(": ");

        if (valid) {
            // 進捗表示: "Ch.X Lv.X"
            line.append("Ch.");
            line.append(bn::to_string<8>(slot.story_chapter + 1));
            line.append(" Lv.");
            line.append(bn::to_string<8>(slot.story_level + 1));
        } else {
            line.append("--- NEW GAME ---");
        }

        text_gen_.generate(0, y, line, sprites_);
    }

    // 操作説明
    text_gen_.generate(0, 60, "A: 選択  B: タイトルへ", sprites_);
}

void SaveSelectState::shutdown() {
    sprites_.clear();
}
