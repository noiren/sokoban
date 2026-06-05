#ifndef PRACTICE_MENU_STATE_H
#define PRACTICE_MENU_STATE_H

#include "state/state.h"
#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "save/user_data_puzzle.h"

enum class PracticeMenuPhase {
    SELECT_LEVEL,
    COUNT
};

// プラクティスモード: ステージ選択画面
class PracticeMenuState : public State {
public:
    PracticeMenuState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}
    void resume(StateManager& /*sm*/, SharedContext& /*ctx*/) override;

private:
    void redraw(SharedContext& ctx);
    void draw_stage_list(SharedContext& ctx);

    bn::vector<bn::sprite_ptr, 64> sprites_;

    UserDataPuzzle user_data_;
    int cursor_;
    int scroll_offset_;  // 表示スクロール用（カーソルに追従）
};

#endif // PRACTICE_MENU_STATE_H
