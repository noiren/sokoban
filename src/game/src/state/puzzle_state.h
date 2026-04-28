#ifndef PUZZLE_STATE_H
#define PUZZLE_STATE_H

#include "state/state.h"
#include "game/sokoban.h"
#include "gfx/renderer.h"
#include "gfx/hud.h"
#include "audio/sound_manager.h"

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_sprite_text_generator.h"
#include "bn_array.h"
#include "bn_optional.h"

class PuzzleState : public State {
public:
    PuzzleState(bn::sprite_text_generator& text_gen, SoundManager& sound);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

    GameState& game_state() { return gs_; }

private:
    bn::sprite_text_generator& text_gen_;
    SoundManager& sound_;
    Hud hud_;
    GameState gs_;

    bn::optional<bn::regular_bg_ptr> bg_;
    bn::optional<bn::regular_bg_map_ptr> bg_map_;
    alignas(4) bn::array<bn::regular_bg_map_cell, 32 * 32> map_cells_;
};

#endif // PUZZLE_STATE_H
