#ifndef ENDLESS_STATE_H
#define ENDLESS_STATE_H

#include "state/state.h"
#include "game/sokoban.h"
#include "gfx/renderer.h"
#include "gfx/hud.h"
#include "audio/sound_manager.h"
#include "save/save_data.h"

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_array.h"
#include "bn_optional.h"
#include "bn_vector.h"

class EndlessState : public State {
public:
    EndlessState(bn::sprite_text_generator& text_gen, SoundManager& sound, SaveSlot& save);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

private:
    bn::sprite_text_generator& text_gen_;
    SoundManager& sound_;
    SaveSlot& save_;
    Hud hud_;
    GameState gs_;

    int score_;           // Current run score
    int difficulty_;      // Current difficulty level
    int seed_;            // Current puzzle seed
    bool show_result_;    // Showing game over / score

    bn::optional<bn::regular_bg_ptr> bg_;
    bn::optional<bn::regular_bg_map_ptr> bg_map_;
    alignas(4) bn::array<bn::regular_bg_map_cell, 32 * 32> map_cells_;

    bn::vector<bn::sprite_ptr, 32> result_sprites_;

    void generate_next();
    void draw_result();
};

#endif // ENDLESS_STATE_H
