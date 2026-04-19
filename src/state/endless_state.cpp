#include "endless_state.h"
#include "state_manager.h"
#include "game/puzzle_gen.h"
#include "bn_keypad.h"
#include "bn_regular_bg_items_bg.h"
#include "bn_string.h"

EndlessState::EndlessState(bn::sprite_text_generator& text_gen, SoundManager& sound, SaveSlot& save)
    : text_gen_(text_gen), sound_(sound), save_(save),
      score_(0), difficulty_(0), seed_(42), show_result_(false) {
}

void EndlessState::init(StateManager& /*manager*/) {
    bg_ = bn::regular_bg_items::bg.create_bg(0, 0);
    bg_map_ = bg_->map();
    hud_.init(text_gen_);

    score_ = 0;
    difficulty_ = 0;
    seed_ = 42;
    show_result_ = false;
    result_sprites_.clear();

    generate_next();
}

void EndlessState::generate_next() {
    // Try to generate a puzzle, retry with different seeds if needed
    bool ok = false;
    for (int attempt = 0; attempt < 20; attempt++) {
        seed_++;
        ok = puzzle_generate(gs_, difficulty_, seed_);
        if (ok) break;
    }

    if (!ok) {
        // Failed to generate, show result
        show_result_ = true;
        draw_result();
        return;
    }

    render_draw_map(map_cells_, *bg_map_, gs_);
}

void EndlessState::update(StateManager& manager) {
    if (show_result_) {
        // Show score screen
        if (bn::keypad::a_pressed() || bn::keypad::start_pressed()) {
            // Save high score
            if (score_ > save_.endless_high_score) {
                save_.endless_high_score = static_cast<uint16_t>(score_);
                // メインloop側でsave_slot_saveが呼ばれる
            }
            manager.pop();
        }
        return;
    }

    bool need_redraw = false;

    if (gs_.cleared) {
        // Puzzle cleared! Increase score and generate next
        score_++;
        sound_.play_clear();

        // Increase difficulty every 3 clears
        difficulty_ = score_ / 3;
        if (difficulty_ > 2) difficulty_ = 2;

        generate_next();
        return;
    }

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

    // R = reset current puzzle
    if (bn::keypad::r_pressed()) {
        generate_next();
        need_redraw = false;  // generate_next already renders
        sound_.play_reset();
    }

    // B = give up, show score
    if (bn::keypad::b_pressed()) {
        show_result_ = true;
        draw_result();
        return;
    }

    if (need_redraw) {
        render_draw_map(map_cells_, *bg_map_, gs_);
    }

    // Draw HUD with score instead of moves
    result_sprites_.clear();
    text_gen_.set_left_alignment();
    bn::string<32> score_text = "SCORE:";
    score_text.append(bn::to_string<8>(score_));
    text_gen_.generate(-120 + 8, 80 - 12, score_text, result_sprites_);

    text_gen_.set_right_alignment();
    bn::string<32> moves_text = "MOVES:";
    moves_text.append(bn::to_string<8>(gs_.moves));
    text_gen_.generate(120 - 8, 80 - 12, moves_text, result_sprites_);
}

void EndlessState::draw_result() {
    result_sprites_.clear();
    text_gen_.set_center_alignment();

    bn::string<32> score_text = "SCORE: ";
    score_text.append(bn::to_string<8>(score_));
    text_gen_.generate(0, -24, score_text, result_sprites_);

    bn::string<32> high_text = "BEST:  ";
    int best = score_ > save_.endless_high_score ? score_ : save_.endless_high_score;
    high_text.append(bn::to_string<8>(best));
    text_gen_.generate(0, -4, high_text, result_sprites_);

    if (score_ > save_.endless_high_score) {
        text_gen_.generate(0, 16, "NEW RECORD!", result_sprites_);
    }

    text_gen_.generate(0, 40, "PRESS A", result_sprites_);
}

void EndlessState::shutdown() {
    result_sprites_.clear();
    hud_.clear();
    bg_map_.reset();
    bg_.reset();
}
