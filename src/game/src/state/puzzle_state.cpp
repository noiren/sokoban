#include "puzzle_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "bn_regular_bg_items_test_bg.h"
#include "ui_data_sokoban_main.h"

PuzzleState::PuzzleState(bn::sprite_text_generator& text_gen, SoundManager& sound)
    : text_gen_(text_gen), sound_(sound),
      ui_manager_(text_gen),
      phase_(PuzzlePhase::PLAYING), step_(PhaseStep::OPENING) {
}

void PuzzleState::init(StateManager& /*manager*/) {
    ui_manager_.load_screen(ui_data_sokoban_main::SCREEN);

    bg_ = bn::regular_bg_items::test_bg.create_bg(0, 0);
    // UIの背景の上にパズルボードを重ねるため、Zオーダーを下げる（手前にする）
    bg_->set_priority(1);

    bg_map_ = bg_->map();
    hud_.init(ui_manager_);

    game_init(gs_, 0);
    render_draw_map(map_cells_, *bg_map_, gs_);

    phase_ = PuzzlePhase::PLAYING;
    step_  = PhaseStep::RUNNING;  // 現時点はフェードなしで即開始
}

void PuzzleState::update(StateManager& manager) {
    switch (step_) {
        case PhaseStep::OPENING:
            // TODO: フェードイン処理
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            switch (phase_) {
                case PuzzlePhase::PLAYING:  update_playing(manager);  break;
                case PuzzlePhase::CLEARED:  update_cleared(manager);  break;
            }
            break;

        case PhaseStep::CLOSING:
            // TODO: フェードアウト処理
            manager.pop();
            break;
    }

    ui_manager_.update();
}

void PuzzleState::update_playing(StateManager& /*manager*/) {
    bool need_redraw = false;

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
            phase_ = PuzzlePhase::CLEARED;
            hud_.draw_clear_message(gs_.current_level + 1);
        }
    }

    if (phase_ == PuzzlePhase::PLAYING) {
        hud_.draw_game_hud(gs_.moves, gs_.current_level + 1);
    }
}

void PuzzleState::update_cleared(StateManager& /*manager*/) {
    if (bn::keypad::start_pressed()) {
        int next = gs_.current_level + 1;
        if (next >= get_num_levels()) next = 0;
        game_init(gs_, next);
        render_draw_map(map_cells_, *bg_map_, gs_);
        hud_.clear();
        phase_ = PuzzlePhase::PLAYING;
    }
}

void PuzzleState::shutdown() {
    hud_.clear();
    bg_map_.reset();
    bg_.reset();
    ui_manager_.clear_all();
}
