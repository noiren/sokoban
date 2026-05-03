#ifndef PRACTICE_MENU_STATE_H
#define PRACTICE_MENU_STATE_H

#include "state/state.h"
#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "../gfx/ui_manager.h"

// プラクティスモード: ステージ選択画面 (TODO: 実装予定)
class PracticeMenuState : public State {
public:
    explicit PracticeMenuState(bn::sprite_text_generator& text_gen);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

private:
    void update_select(StateManager& manager);

    UIManager ui_manager_;
    PhaseStep step_;
};

#endif // PRACTICE_MENU_STATE_H
