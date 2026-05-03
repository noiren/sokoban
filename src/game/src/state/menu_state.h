#ifndef MENU_STATE_H
#define MENU_STATE_H

#include "state/state.h"
#include "audio/sound_manager.h"
#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "../gfx/ui_manager.h"

class MenuUI {
public:
    MenuUI(UIManager& ui) : ui_(ui) {}
    void set_menu_item(int index, const bn::string_view& text) {
        if (index == 0) ui_.set_text("menu_0", text);
        else if (index == 1) ui_.set_text("menu_1", text);
        else if (index == 2) ui_.set_text("menu_2", text);
        else if (index == 3) ui_.set_text("menu_3", text);
        else if (index == 4) ui_.set_text("menu_4", text);
    }
private:
    UIManager& ui_;
};

// メインメニューの選択肢
// 設計書に合わせてプラクティスとギャラリーを追加
// DEBUG は非表示（SELECTボタンで隠しアクセス）
enum class MenuItem {
    STORY,
    PRACTICE,
    ENDLESS,
    GALLERY,
    SETTINGS,
    DEBUG,   // 非表示隠しコマンド
    COUNT
};

// 表示する項目数（DEBUGを除く）
constexpr int VISIBLE_MENU_COUNT = 5;

class MenuState : public State {
public:
    MenuState(bn::sprite_text_generator& text_gen, SoundManager& sound);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

    MenuItem last_selected() const { return last_selected_; }

private:
    void update_menu_ui();

    bn::sprite_text_generator& text_gen_;
    SoundManager& sound_;
    int cursor_;
    MenuItem last_selected_;
    UIManager ui_manager_;
    bn::optional<MenuUI> ui_;
};

#endif // MENU_STATE_H
