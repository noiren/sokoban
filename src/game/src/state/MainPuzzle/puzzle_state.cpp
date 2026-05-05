#include "puzzle_state.h"
#include "state/Manager/state_manager.h"
#include "game/sokoban.h"
#include "bn_keypad.h"
#include "bn_regular_bg_items_test_bg.h"
#include "bn_string.h"
#include "ui_data_sokoban_main.h"

PuzzleState::PuzzleState()
    : step_(PhaseStep::OPENING),
      phase_(PuzzlePhase::PLAYING) {
}

void PuzzleState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    current_level_ = 0;
    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_sokoban_main::SCREEN);

    bg_ = bn::regular_bg_items::test_bg.create_bg(0, 0);
    bg_->set_priority(1);

    bg_map_ = bg_->map();
    hud_.init(*ui_manager_);
    hud_.set_level(current_level_);

    level_init();

    phase_ = PuzzlePhase::PLAYING;
    step_  = PhaseStep::RUNNING;
}

void PuzzleState::level_init() {
    game_init(gs_, current_level_);
    render_draw_map(map_cells_, *bg_map_, gs_);
    moves_ = 0;
    update_hud();
}

void PuzzleState::update_hud() {
    if (!ui_manager_) return;
    bn::string<32> level_text = "STAGE ";
    level_text.append(bn::to_string<4>(current_level_ + 1));
    ui_manager_->set_text("stage_text", level_text);

    bn::string<32> moves_text = "MOVES: ";
    moves_text.append(bn::to_string<8>(gs_.moves));
    ui_manager_->set_text("moves_text", moves_text);
}

void PuzzleState::update(StateManager& sm, SharedContext& ctx) {
    switch (step_) {
        case PhaseStep::OPENING:
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            switch (phase_) {
                case PuzzlePhase::PLAYING: update_playing(sm, ctx); break;
                case PuzzlePhase::CLEARED: update_cleared(sm, ctx); break;
            }
            break;

        case PhaseStep::CLOSING:
            break;
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void PuzzleState::update_playing(StateManager& sm, SharedContext& ctx) {
    // クリア済みフラグが立っていたらクリアフェーズへ
    if (gs_.cleared) {
        phase_ = PuzzlePhase::CLEARED;
        if (ctx.sound) ctx.sound->play_clear();
        draw_clear_text(ctx);
        return;
    }

    // 操作
    bool moved = false;
    if (bn::keypad::up_pressed())    { game_move(gs_, 0, -1); moved = true; }
    if (bn::keypad::down_pressed())  { game_move(gs_, 0,  1); moved = true; }
    if (bn::keypad::left_pressed())  { game_move(gs_, -1, 0); moved = true; }
    if (bn::keypad::right_pressed()) { game_move(gs_,  1, 0); moved = true; }

    if (gs_.moves != moves_) {
        if (ctx.sound) ctx.sound->play_move();
        moves_ = gs_.moves;
    }

    // L = リセット
    if (bn::keypad::l_pressed()) {
        level_init();
        if (ctx.sound) ctx.sound->play_reset();
        return;
    }

    // R = リトライ (リセットと同じ)
    if (bn::keypad::r_pressed()) {
        level_init();
        if (ctx.sound) ctx.sound->play_reset();
        return;
    }

    // SELECT = メニューに戻る
    if (bn::keypad::select_pressed()) {
        sm.change_state(StateID::MENU);
        return;
    }

    if (moved) {
        render_draw_map(map_cells_, *bg_map_, gs_);
        update_hud();
    }
}

void PuzzleState::update_cleared(StateManager& sm, SharedContext& ctx) {
    if (bn::keypad::a_pressed()) {
        current_level_++;
        if (current_level_ >= get_num_levels()) {
            current_level_ = 0;  // 最後までクリアしたらループ
        }
        level_init();
        phase_ = PuzzlePhase::PLAYING;
    }
}

void PuzzleState::draw_clear_text(SharedContext& /*ctx*/) {
    if (ui_manager_) {
        ui_manager_->set_text("endless_score_label", "CLEAR!");
        ui_manager_->set_text_visible("endless_score_label", true);
    }
}

void PuzzleState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    hud_.clear();
    bg_map_.reset();
    bg_.reset();
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}
