#include "endless_state.h"
#include "state_manager.h"
#include "game/puzzle_gen.h"
#include "bn_keypad.h"
#include "bn_regular_bg_items_test_bg.h"
#include "bn_string.h"
#include "ui_data_sokoban_main.h"

EndlessState::EndlessState(bn::sprite_text_generator& text_gen, SoundManager& sound, SaveSlot& save)
    : text_gen_(text_gen), sound_(sound), save_(save),
      score_(0), difficulty_(0), seed_(42),
      ui_manager_(text_gen),
      phase_(EndlessPhase::PLAYING), step_(PhaseStep::OPENING) {
}

void EndlessState::init(StateManager& /*manager*/) {
    ui_manager_.load_screen(ui_data_sokoban_main::SCREEN);

    bg_ = bn::regular_bg_items::test_bg.create_bg(0, 0);
    // UI背景の上にパズルを重ねるため優先度を下げる
    bg_->set_priority(1);

    bg_map_ = bg_->map();
    hud_.init(ui_manager_);

    score_      = 0;
    difficulty_ = 0;
    seed_       = 42;
    result_sprites_.clear();

    generate_next();

    phase_ = EndlessPhase::PLAYING;
    step_  = PhaseStep::RUNNING;  // 現時点はフェードなしで即開始
}

void EndlessState::generate_next() {
    bool ok = false;
    for (int attempt = 0; attempt < 20; attempt++) {
        seed_++;
        ok = puzzle_generate(gs_, difficulty_, seed_);
        if (ok) break;
    }

    if (!ok) {
        // 生成失敗 → リザルトへ
        phase_ = EndlessPhase::RESULT;
        draw_result();
        return;
    }

    render_draw_map(map_cells_, *bg_map_, gs_);
}

void EndlessState::update(StateManager& manager) {
    switch (step_) {
        case PhaseStep::OPENING:
            // TODO: フェードイン処理
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            switch (phase_) {
                case EndlessPhase::PLAYING: update_playing(manager); break;
                case EndlessPhase::RESULT:  update_result(manager);  break;
            }
            break;

        case PhaseStep::CLOSING:
            // TODO: フェードアウト処理
            manager.pop();
            break;
    }

    ui_manager_.update();
}

void EndlessState::update_playing(StateManager& /*manager*/) {
    if (gs_.cleared) {
        score_++;
        sound_.play_clear();

        difficulty_ = score_ / 3;
        if (difficulty_ > 2) difficulty_ = 2;

        generate_next();
        return;
    }

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

    // R = 現在のパズルをリセット
    if (bn::keypad::r_pressed()) {
        generate_next();
        sound_.play_reset();
        return;
    }

    // B = ギブアップ → リザルト表示
    if (bn::keypad::b_pressed()) {
        phase_ = EndlessPhase::RESULT;
        draw_result();
        return;
    }

    if (need_redraw) {
        render_draw_map(map_cells_, *bg_map_, gs_);
    }

    // HUDにスコアと手数を表示
    bn::string<32> score_text = "SCORE:";
    score_text.append(bn::to_string<8>(score_));
    ui_manager_.set_text("moves_text", score_text);

    bn::string<32> moves_text = "MOVES:";
    moves_text.append(bn::to_string<8>(gs_.moves));
    ui_manager_.set_text("stage_text", moves_text);
}

void EndlessState::update_result(StateManager& /*manager*/) {
    if (bn::keypad::a_pressed() || bn::keypad::start_pressed()) {
        // ハイスコア保存（セーブ自体はmain.cpp側で行う）
        if (score_ > save_.endless_high_score) {
            save_.endless_high_score = static_cast<uint16_t>(score_);
        }
        step_ = PhaseStep::CLOSING;
    }
}

void EndlessState::draw_result() {
    bn::string<32> score_text = "SCORE: ";
    score_text.append(bn::to_string<8>(score_));
    ui_manager_.set_text("endless_score_label", score_text);
    ui_manager_.set_text_visible("endless_score_label", true);

    bn::string<32> high_text = "BEST:  ";
    int best = score_ > save_.endless_high_score ? score_ : save_.endless_high_score;
    high_text.append(bn::to_string<8>(best));
    ui_manager_.set_text("endless_best_label", high_text);
    ui_manager_.set_text_visible("endless_best_label", true);

    if (score_ > save_.endless_high_score) {
        ui_manager_.set_text("endless_new_record", "NEW RECORD!");
        ui_manager_.set_text_visible("endless_new_record", true);
    }

    ui_manager_.set_text("endless_press_a", "PRESS A");
    ui_manager_.set_text_visible("endless_press_a", true);
}

void EndlessState::shutdown() {
    hud_.clear();
    bg_map_.reset();
    bg_.reset();
    ui_manager_.clear_all();
}
