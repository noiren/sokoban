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
        if (index == 0) {
            if (auto* text_node = ui_.get_text("menu_0")) text_node->set_text(text);
        } else if (index == 1) {
            if (auto* text_node = ui_.get_text("menu_1")) text_node->set_text(text);
        } else if (index == 2) {
            if (auto* text_node = ui_.get_text("menu_2")) text_node->set_text(text);
        } else if (index == 3) {
            if (auto* text_node = ui_.get_text("menu_3")) text_node->set_text(text);
        } else if (index == 4) {
            if (auto* text_node = ui_.get_text("menu_4")) text_node->set_text(text);
        }
    }
private:
    UIManager& ui_;
};

enum class MenuItem {
    STORY,
    PRACTICE,
    ENDLESS,
    GALLERY,
    SETTINGS,
    DEBUG,   // 非表示隠しコマンド
    COUNT
};

constexpr int VISIBLE_MENU_COUNT = 5;

enum class MenuPhase {
    MAIN,
    COUNT
};

class MenuState : public State {
public:
    MenuState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void change_phase(MenuPhase next);

    void enter_main();
    void update_main(StateManager& sm, SharedContext& ctx);
    void exit_main();

    void update_menu_ui();

    using EnterExitFunc = void (MenuState::*)();
    using UpdateFunc = void (MenuState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];

    int cursor_;
    MenuItem last_selected_;
    bn::optional<UIManager> ui_manager_;
    bn::optional<MenuUI> ui_;
    MenuPhase phase_;
    PhaseStep step_;
};

#endif // MENU_STATE_H
