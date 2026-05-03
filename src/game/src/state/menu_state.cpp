#include "menu_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "ui_data_mainmenu.h"

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
      cursor_(0), last_selected_(MenuItem::STORY), ui_manager_(text_gen) {
}

void MenuState::init(StateManager& /*manager*/) {
    cursor_ = 0;
    
    ui_manager_.load_screen(ui_data_mainmenu::SCREEN);
    ui_.emplace(ui_manager_);

    update_menu_ui();
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
        update_menu_ui();
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

    ui_manager_.update();
}

void MenuState::update_menu_ui() {
    if (!ui_) return;

    for (int i = 0; i < VISIBLE_MENU_COUNT; i++) {
        if (i == cursor_) {
            bn::string<48> line = "> ";
            line.append(MENU_LABELS[i]);
            ui_->set_menu_item(i, line);
        } else {
            ui_->set_menu_item(i, MENU_LABELS[i]);
        }
    }
}

void MenuState::shutdown() {
    ui_manager_.clear_all();
    ui_.reset();
}
