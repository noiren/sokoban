#include "menu_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"

// メニュー表示ラベル（DIBUGは非表示、VISIBLE_MENU_COUNTまで表示）
namespace {
    const char* MENU_LABELS[] = {
        "ストーリーモード",
        "プラクティス",
        "エンドレスモード",
        "ギャラリー",
        "設定",
    };
}

MenuState::MenuState(bn::sprite_text_generator& text_gen, SoundManager& sound)
    : text_gen_(text_gen), sound_(sound),
      cursor_(0), last_selected_(MenuItem::STORY) {
}

void MenuState::init(StateManager& /*manager*/) {
    cursor_ = 0;
    sprites_.clear();
    draw_menu();
}

void MenuState::update(StateManager& manager) {
    bool changed = false;

    if (bn::keypad::up_pressed()) {
        cursor_--;
        if (cursor_ < 0) cursor_ = VISIBLE_MENU_COUNT - 1;
        changed = true;
        sound_.play_move();
    }
    if (bn::keypad::down_pressed()) {
        cursor_++;
        if (cursor_ >= VISIBLE_MENU_COUNT) cursor_ = 0;
        changed = true;
        sound_.play_move();
    }

    if (changed) {
        draw_menu();
    }

    if (bn::keypad::a_pressed()) {
        last_selected_ = static_cast<MenuItem>(cursor_);
        manager.pop();
    }

    // 隠しコマンド: SELECT → デバッグメニュー
    if (bn::keypad::select_pressed()) {
        last_selected_ = MenuItem::DEBUG;
        manager.pop();
    }
}

void MenuState::draw_menu() {
    sprites_.clear();
    text_gen_.set_center_alignment();

    // タイトル
    text_gen_.generate(0, -56, "ｽﾋﾟｷとマヨの倉庫番", sprites_);

    for (int i = 0; i < VISIBLE_MENU_COUNT; i++) {
        int y = -32 + i * 18;
        if (i == cursor_) {
            bn::string<48> line = "> ";
            line.append(MENU_LABELS[i]);
            text_gen_.generate(0, y, line, sprites_);
        } else {
            text_gen_.generate(0, y, MENU_LABELS[i], sprites_);
        }
    }
}

void MenuState::shutdown() {
    sprites_.clear();
}
