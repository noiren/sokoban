#ifndef PRACTICE_MENU_STATE_H
#define PRACTICE_MENU_STATE_H

#include "state/state.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

// プラクティスモード: ステージ選択画面 (TODO: 実装予定)
class PracticeMenuState : public State {
public:
    explicit PracticeMenuState(bn::sprite_text_generator& text_gen);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

private:
    bn::sprite_text_generator& text_gen_;
    bn::vector<bn::sprite_ptr, 32> sprites_;
};

#endif // PRACTICE_MENU_STATE_H
