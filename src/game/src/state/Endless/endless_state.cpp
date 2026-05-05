#include "endless_state.h"
#include "state/Manager/state_manager.h"
#include "game/puzzle_gen.h"
#include "bn_keypad.h"
#include "bn_regular_bg_items_test_bg.h"
#include "bn_string.h"
#include "ui_data_sokoban_main.h"

EndlessState::EndlessState()
    : score_(0), difficulty_(0), seed_(42),
      phase_(EndlessPhase::PLAYING), step_(PhaseStep::OPENING) {
}

void EndlessState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_sokoban_main::SCREEN);

    bg_ = bn::regular_bg_items::test_bg.create_bg(0, 0);
    // UI背景の上にパズルを重ねるため優先度を下げる
    bg_->set_priority(1);

    bg_map_ = bg_->map();
    hud_.init(*ui_manager_);

    score_      = 0;
    difficulty_ = 0;
    seed_       = 42;
    result_sprites_.clear();

    generate_next();

    phase_ = EndlessPhase::PLAYING;
    step_  = PhaseStep::RUNNING;
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
        // draw_result needs ctx, but generate_next is internal. 
        // I'll call draw_result in update instead or pass ctx.
    }

    render_draw_map(map_cells_, *bg_map_, gs_);
}

void EndlessState::update(StateManager& sm, SharedContext& ctx) {
    switch (step_) {
        case PhaseStep::OPENING:
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            if (phase_ == EndlessPhase::RESULT && result_sprites_.empty()) {
                 draw_result(ctx);
            }
            switch (phase_) {
                case EndlessPhase::PLAYING: update_playing(sm, ctx); break;
                case EndlessPhase::RESULT:  update_result(sm, ctx);  break;
            }
            break;

        case PhaseStep::CLOSING:
            break;
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void EndlessState::update_playing(StateManager& sm, SharedContext& ctx) {
    if (gs_.cleared) {
        score_++;
        if (ctx.sound) ctx.sound->play_clear();

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
        if (ctx.sound) ctx.sound->play_move();
    }

    // R = 現在のパズルをリセット
    if (bn::keypad::r_pressed()) {
        generate_next();
        if (ctx.sound) ctx.sound->play_reset();
        return;
    }

    // B = ギブアップ → リザルト表示
    if (bn::keypad::b_pressed()) {
        phase_ = EndlessPhase::RESULT;
        draw_result(ctx);
        return;
    }

    // SELECT = メニューに戻る
    if (bn::keypad::select_pressed()) {
        sm.change_state(StateID::MENU);
        return;
    }

    if (need_redraw) {
        render_draw_map(map_cells_, *bg_map_, gs_);
    }

    if (ui_manager_) {
        bn::string<32> score_text = "SCORE:";
        score_text.append(bn::to_string<8>(score_));
        ui_manager_->set_text("moves_text", score_text);

        bn::string<32> moves_text = "MOVES:";
        moves_text.append(bn::to_string<8>(gs_.moves));
        ui_manager_->set_text("stage_text", moves_text);
    }
}

void EndlessState::update_result(StateManager& sm, SharedContext& ctx) {
    if (bn::keypad::a_pressed() || bn::keypad::start_pressed()) {
        SaveSlot& save = ctx.save->slots[ctx.active_slot];
        if (score_ > save.endless_high_score) {
            save.endless_high_score = static_cast<uint16_t>(score_);
        }
        sm.change_state(StateID::MENU);
    }
}

void EndlessState::draw_result(SharedContext& ctx) {
    if (!ui_manager_) return;
    SaveSlot& save = ctx.save->slots[ctx.active_slot];

    bn::string<32> score_text = "SCORE: ";
    score_text.append(bn::to_string<8>(score_));
    ui_manager_->set_text("endless_score_label", score_text);
    ui_manager_->set_text_visible("endless_score_label", true);

    bn::string<32> high_text = "BEST:  ";
    int best = score_ > save.endless_high_score ? score_ : save.endless_high_score;
    high_text.append(bn::to_string<8>(best));
    ui_manager_->set_text("endless_best_label", high_text);
    ui_manager_->set_text_visible("endless_best_label", true);

    if (score_ > save.endless_high_score) {
        ui_manager_->set_text("endless_new_record", "NEW RECORD!");
        ui_manager_->set_text_visible("endless_new_record", true);
    }

    ui_manager_->set_text("endless_press_a", "PRESS A");
    ui_manager_->set_text_visible("endless_press_a", true);
}

void EndlessState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    hud_.clear();
    bg_map_.reset();
    bg_.reset();
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}
