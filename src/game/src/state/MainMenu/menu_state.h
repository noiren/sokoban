#ifndef MENU_STATE_H
#define MENU_STATE_H

#include "state/state.h"
#include "audio/sound_manager.h"
#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "gfx/ui_manager.h"

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
    MenuState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void update_menu(StateManager& sm, SharedContext& ctx);
    void update_menu_ui();

    int cursor_;
    MenuItem last_selected_;
    bn::optional<UIManager> ui_manager_;
    bn::optional<MenuUI> ui_;
    PhaseStep step_;
};

#endif // MENU_STATE_H
