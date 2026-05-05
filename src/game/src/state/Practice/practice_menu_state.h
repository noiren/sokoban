#ifndef PRACTICE_MENU_STATE_H
#define PRACTICE_MENU_STATE_H

#include "state/state.h"
#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "gfx/ui_manager.h"

// プラクティスモード: ステージ選択画面 (TODO: 実装予定)
class PracticeMenuState : public State {
public:
    PracticeMenuState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void update_menu(StateManager& sm, SharedContext& ctx);

    bn::optional<UIManager> ui_manager_;
    int cursor_;
    int selected_level_;
    PhaseStep step_;
};

#endif // PRACTICE_MENU_STATE_H
