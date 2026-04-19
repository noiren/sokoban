#include "puzzle_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "bn_regular_bg_items_bg.h"

PuzzleState::PuzzleState(bn::sprite_text_generator& text_gen, SoundManager& sound)
    : text_gen_(text_gen), sound_(sound) {
}

void PuzzleState::init(StateManager& /*manager*/) {
    bg_ = bn::regular_bg_items::bg.create_bg(0, 0);
    bg_map_ = bg_->map();
    hud_.init(text_gen_);

    game_init(gs_, 0);
    render_draw_map(map_cells_, *bg_map_, gs_);
}

void PuzzleState::update(StateManager& /*manager*/) {
    bool need_redraw = false;

    if (gs_.cleared) {
        hud_.draw_clear_message(gs_.current_level + 1);

        if (bn::keypad::start_pressed()) {
            int next = gs_.current_level + 1;
            if (next >= get_num_levels()) next = 0;
            game_init(gs_, next);
            render_draw_map(map_cells_, *bg_map_, gs_);
            hud_.clear();
        }
    } else {
        int old_moves = gs_.moves;

        if (bn::keypad::up_pressed()) {
            game_move(gs_, 0, -1);
            need_redraw = true;
        } else if (bn::keypad::down_pressed()) {
            game_move(gs_, 0, 1);
            need_redraw = true;
        } else if (bn::keypad::left_pressed()) {
            game_move(gs_, -1, 0);
            need_redraw = true;
        } else if (bn::keypad::right_pressed()) {
            game_move(gs_, 1, 0);
            need_redraw = true;
        }

        if (gs_.moves != old_moves) {
            sound_.play_move();
        }

        if (bn::keypad::r_pressed()) {
            game_init(gs_, gs_.current_level);
            need_redraw = true;
            sound_.play_reset();
        }

        if (need_redraw) {
            render_draw_map(map_cells_, *bg_map_, gs_);

            if (gs_.cleared) {
                sound_.play_clear();
            }
        }

        hud_.draw_game_hud(gs_.moves, gs_.current_level + 1);
    }
}

void PuzzleState::shutdown() {
    hud_.clear();
    bg_map_.reset();
    bg_.reset();
}
