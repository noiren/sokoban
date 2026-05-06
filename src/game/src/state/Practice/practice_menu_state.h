#ifndef PRACTICE_MENU_STATE_H
#define PRACTICE_MENU_STATE_H

#include "state/state.h"
#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "gfx/ui_manager.h"

enum class PracticeMenuPhase {
    SELECT_LEVEL,
    COUNT
};

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
    void change_phase(PracticeMenuPhase next);

    void enter_select();
    void update_select(StateManager& sm, SharedContext& ctx);
    void exit_select();

    using EnterExitFunc = void (PracticeMenuState::*)();
    using UpdateFunc = void (PracticeMenuState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];

    bn::optional<UIManager> ui_manager_;
    int cursor_;
    int selected_level_;
    PracticeMenuPhase phase_;
    PhaseStep step_;
};

#endif // PRACTICE_MENU_STATE_H
