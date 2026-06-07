#include "endless_state.h"

#include "state/Manager/state_manager.h"
#include "state/state_id.h"
#include "input/input_manager.h"
#include "graphics/renderer.h"
#include "ui_data_sokoban_main.h"

#include "bn_regular_bg_items_still_puzzle_map.h"
#include "bn_string.h"
#include "bn_keypad.h"
#include "bn_assert.h"
#include "bn_core.h"
#include "bn_sprite_items_spr_player.h"
#include "save/save_data.h"
#include "ui/Core/Components/ui_image.h"

namespace {

constexpr int k_max_gen_attempts = 100;
constexpr int k_difficulty_ramp_every = 3;
constexpr int k_max_difficulty      = 2;

} // namespace

const EndlessState::PhaseHandlers EndlessState::phase_table_[] = {
    { &EndlessState::enter_playing, &EndlessState::update_playing, &EndlessState::exit_playing },
    { &EndlessState::enter_result, &EndlessState::update_result, &EndlessState::exit_result },
};

const EndlessState::PuzzlePhaseHandlers EndlessState::puzzle_phase_table_[] = {
    { &EndlessState::enter_pp_playing, &EndlessState::update_pp_playing, &EndlessState::exit_pp_playing },
    { &EndlessState::enter_pp_animating, &EndlessState::update_pp_animating, &EndlessState::exit_pp_animating },
    { &EndlessState::enter_pp_failed, &EndlessState::update_pp_failed, &EndlessState::exit_pp_failed },
    { &EndlessState::enter_pp_cleared, &EndlessState::update_pp_cleared, &EndlessState::exit_pp_cleared },
};

EndlessState::EndlessState()
    : phase_(EndlessPhase::PLAYING),
      puzzle_phase_(EndlessPuzzlePhase::PP_PLAYING),
      step_(PhaseStep::OPENING),
      last_result_(PuzzleEngine::Result::CONTINUE),
      score_(0),
      difficulty_(0),
      seed_(1),
      last_drawn_moves_(-1),
      current_event_index_(0) {
}

void EndlessState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    BN_ASSERT(ctx.text_generator != nullptr, "EndlessState::enter: text_generator is null");

    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_sokoban_main::SCREEN);

    score_            = 0;
    difficulty_     = 0;
    last_drawn_moves_ = -1;
    player_dir_       = 0;

    unsigned tick = static_cast<unsigned>(bn::core::current_cpu_ticks() & 0x7FFFFFFFu);
    seed_            = (tick == 0) ? 1 : static_cast<int>(tick);

    phase_        = EndlessPhase::PLAYING;
    puzzle_phase_ = EndlessPuzzlePhase::PP_PLAYING;

    ensure_map_resources(ctx);
    generate_next();
    refresh_rival_portrait_ui();

    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void EndlessState::update(StateManager& sm, SharedContext& ctx) {
    if (phase_ == EndlessPhase::PLAYING) {
        update_hud();
    }

    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    if (phase_ == EndlessPhase::PLAYING) {
        update_camera();
        update_player_sprite();
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void EndlessState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    bg_.reset();
    bg_map_.reset();
    camera_.reset();
    player_sprite_.reset();
    player_anim_.reset();

    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}

void EndlessState::change_phase(EndlessPhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    phase_ = next;

    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void EndlessState::change_puzzle_phase(EndlessPuzzlePhase next) {
    const int pi = static_cast<int>(puzzle_phase_);
    BN_ASSERT(pi >= 0 && pi < static_cast<int>(EndlessPuzzlePhase::COUNT),
              "EndlessState::change_puzzle_phase: bad current phase");

    if (puzzle_phase_table_[pi].exit) {
        (this->*puzzle_phase_table_[pi].exit)();
    }

    puzzle_phase_ = next;

    const int ni = static_cast<int>(puzzle_phase_);
    BN_ASSERT(ni >= 0 && ni < static_cast<int>(EndlessPuzzlePhase::COUNT),
              "EndlessState::change_puzzle_phase: bad next phase");

    if (puzzle_phase_table_[ni].enter) {
        (this->*puzzle_phase_table_[ni].enter)();
    }
}

// =============================================================================
// 大フェーズ：PLAYING
// =============================================================================

void EndlessState::enter_playing() {
    step_         = PhaseStep::RUNNING;
    puzzle_phase_ = EndlessPuzzlePhase::PP_PLAYING;
    if (puzzle_phase_table_[static_cast<int>(puzzle_phase_)].enter) {
        (this->*puzzle_phase_table_[static_cast<int>(puzzle_phase_)].enter)();
    }
}

void EndlessState::update_playing(StateManager& sm, SharedContext& ctx) {
    const int pi = static_cast<int>(puzzle_phase_);
    BN_ASSERT(pi >= 0 && pi < static_cast<int>(EndlessPuzzlePhase::COUNT),
              "EndlessState::update_playing: puzzle phase OOB");

    if (puzzle_phase_table_[pi].update) {
        (this->*puzzle_phase_table_[pi].update)(sm, ctx);
    }
}

void EndlessState::exit_playing() {
    const int pi = static_cast<int>(puzzle_phase_);
    if (pi >= 0 && pi < static_cast<int>(EndlessPuzzlePhase::COUNT)) {
        if (puzzle_phase_table_[pi].exit) {
            (this->*puzzle_phase_table_[pi].exit)();
        }
    }
}

// =============================================================================
// 大フェーズ：RESULT
// =============================================================================

void EndlessState::enter_result() {
    step_ = PhaseStep::RUNNING;
    if (!ui_manager_) {
        return;
    }
    if (auto* img = ui_manager_->get_image("rival_portrait")) {
        img->set_visible(false);
    }
    if (auto* node = ui_manager_->get_text("clear_text")) {
        node->set_text("GAME OVER");
        node->set_visible(true);
    }
    if (auto* node = ui_manager_->get_text("start_text")) {
        bn::string<48> msg = "SCORE:";
        msg.append(bn::to_string<8>(score_));
        msg.append(" A:MENU");
        node->set_text(msg);
        node->set_visible(true);
    }
}

void EndlessState::update_result(StateManager& sm, SharedContext& ctx) {
    if (InputManager::instance().is_triggered(Action::Decide) ||
        InputManager::instance().is_triggered(Action::Cancel)) {

        if (ctx.save) {
            BN_ASSERT(ctx.active_slot >= 0 && ctx.active_slot < NUM_SAVE_SLOTS,
                      "EndlessState::update_result: active_slot out of range");
            SaveSlot& slot = ctx.save->slots[ctx.active_slot];
            const uint16_t cleared = static_cast<uint16_t>(score_);
            if (cleared > slot.endless_high_score) {
                slot.endless_high_score = cleared;
                save_data_save(*ctx.save);
            }
        }

        sm.change_state(StateID::MENU);
    }
}

void EndlessState::exit_result() {
    if (!ui_manager_) {
        return;
    }
    if (auto* node = ui_manager_->get_text("clear_text")) {
        node->set_visible(false);
    }
    if (auto* node = ui_manager_->get_text("start_text")) {
        node->set_visible(false);
    }
}

// =============================================================================
// 内フェーズ：PP_PLAYING
// =============================================================================

void EndlessState::enter_pp_playing() {
    if (ui_manager_) {
        if (auto* node = ui_manager_->get_text("start_text")) {
            node->set_visible(false);
        }
    }
}

void EndlessState::update_pp_playing(StateManager& sm, SharedContext& /*ctx*/) {
    if (InputManager::instance().is_triggered(Action::Cancel)) {
        sm.change_state(StateID::MENU);
        return;
    }

    if (bn::keypad::select_pressed() || bn::keypad::r_pressed() || bn::keypad::start_pressed()) {
        reload_last_generated();
        change_puzzle_phase(EndlessPuzzlePhase::PP_PLAYING);
        return;
    }

    if (bn::keypad::l_pressed()) {
        if (engine_.try_undo()) {
            redraw_map();
            return;
        }
    }

    int dx = 0, dy = 0;
    if (InputManager::instance().is_triggered(Action::MoveUp)) {
        dy = -1;
    } else if (InputManager::instance().is_triggered(Action::MoveDown)) {
        dy = 1;
    } else if (InputManager::instance().is_triggered(Action::MoveLeft)) {
        dx = -1;
    } else if (InputManager::instance().is_triggered(Action::MoveRight)) {
        dx = 1;
    }

    if (dx == 0 && dy == 0) {
        return;
    }

    int old_px = engine_.data().player_x;
    int old_py = engine_.data().player_y;

    last_result_ = engine_.try_move(dx, dy);

    if (dx < 0) {
        player_dir_ = 2;
    }
    if (dx > 0) {
        player_dir_ = 3;
    }
    if (dy < 0) {
        player_dir_ = 1;
    }
    if (dy > 0) {
        player_dir_ = 0;
    }

    if (engine_.data().player_x != old_px || engine_.data().player_y != old_py) {
        move_src_x_ = old_px;
        move_src_y_ = old_py;
        move_dst_x_ = engine_.data().player_x;
        move_dst_y_ = engine_.data().player_y;
        move_anim_frames_ = 0;

        if (player_sprite_) {
            player_anim_.emplace(bn::create_sprite_animate_action_forever(
                *player_sprite_, 3, bn::sprite_items::spr_player.tiles_item(),
                player_dir_ * 3 + 1, player_dir_ * 3 + 0, player_dir_ * 3 + 2, player_dir_ * 3 + 0));
        }

        change_puzzle_phase(EndlessPuzzlePhase::PP_ANIMATING);
    } else if (!engine_.events().empty()) {
        change_puzzle_phase(EndlessPuzzlePhase::PP_ANIMATING);
    } else {
        if (last_result_ == PuzzleEngine::Result::FAILED) {
            change_puzzle_phase(EndlessPuzzlePhase::PP_FAILED);
        }
        if (last_result_ == PuzzleEngine::Result::CLEARED) {
            change_puzzle_phase(EndlessPuzzlePhase::PP_CLEARED);
        }
    }
}

void EndlessState::exit_pp_playing() {}

// =============================================================================
// 内フェーズ：PP_ANIMATING
// =============================================================================

void EndlessState::enter_pp_animating() {
    current_event_index_ = 0;
    redraw_map();
}

void EndlessState::update_pp_animating(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (move_anim_frames_ < move_anim_max_frames_) {
        ++move_anim_frames_;
        return;
    }

    const auto& events = engine_.events();
    current_event_index_ = events.size();

    if (current_event_index_ >= events.size()) {
        engine_.clear_events();

        if (last_result_ == PuzzleEngine::Result::FAILED) {
            change_puzzle_phase(EndlessPuzzlePhase::PP_FAILED);
        } else if (last_result_ == PuzzleEngine::Result::CLEARED) {
            change_puzzle_phase(EndlessPuzzlePhase::PP_CLEARED);
        } else {
            if (player_sprite_) {
                player_anim_.emplace(bn::create_sprite_animate_action_forever(
                    *player_sprite_, 10, bn::sprite_items::spr_player.tiles_item(),
                    player_dir_ * 3, player_dir_ * 3, player_dir_ * 3, player_dir_ * 3));
            }
            change_puzzle_phase(EndlessPuzzlePhase::PP_PLAYING);
        }
    }
}

void EndlessState::exit_pp_animating() {}

// =============================================================================
// 内フェーズ：PP_FAILED
// =============================================================================

void EndlessState::enter_pp_failed() {
    if (ui_manager_) {
        if (auto* node = ui_manager_->get_text("start_text")) {
            node->set_text("FAILED! R/SEL:RETRY B:MENU");
            node->set_visible(true);
        }
    }
}

void EndlessState::update_pp_failed(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (InputManager::instance().is_triggered(Action::Cancel)) {
        change_phase(EndlessPhase::RESULT);
        return;
    }

    if (bn::keypad::select_pressed() || bn::keypad::r_pressed()) {
        reload_last_generated();
        change_puzzle_phase(EndlessPuzzlePhase::PP_PLAYING);
    }
}

void EndlessState::exit_pp_failed() {
    if (ui_manager_) {
        if (auto* node = ui_manager_->get_text("start_text")) {
            node->set_visible(false);
        }
    }
}

// =============================================================================
// 内フェーズ：PP_CLEARED
// =============================================================================

void EndlessState::enter_pp_cleared() {
    if (ui_manager_) {
        if (auto* node = ui_manager_->get_text("clear_text")) {
            if (engine_.data().dropped_barrels == 0) {
                node->set_text("PERFECT!");
            } else {
                node->set_text("CLEAR!");
            }
            node->set_visible(true);
        }
        if (auto* node = ui_manager_->get_text("start_text")) {
            node->set_text("A: NEXT STAGE");
            node->set_visible(true);
        }
    }
}

void EndlessState::update_pp_cleared(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (InputManager::instance().is_triggered(Action::Decide) || bn::keypad::start_pressed()) {
        ++score_;
        if (score_ % k_difficulty_ramp_every == 0 && difficulty_ < k_max_difficulty) {
            ++difficulty_;
        }
        generate_next();
        change_puzzle_phase(EndlessPuzzlePhase::PP_PLAYING);
    }
}

void EndlessState::exit_pp_cleared() {
    if (ui_manager_) {
        if (auto* node = ui_manager_->get_text("clear_text")) {
            node->set_visible(false);
        }
        if (auto* node = ui_manager_->get_text("start_text")) {
            node->set_visible(false);
        }
    }
}

// =============================================================================
// 盤面生成・描画
// =============================================================================

void EndlessState::generate_next() {
    for (int attempt = 0; attempt < k_max_gen_attempts; ++attempt) {
        ++seed_;
        if (puzzle_generate(last_generated_, difficulty_, seed_)) {
            engine_.load_from_game_state(last_generated_);
            last_drawn_moves_ = -1;
            player_dir_       = 0;
            redraw_map();
            refresh_rival_portrait_ui();
            return;
        }
    }

    difficulty_ = 0;
    for (int attempt = 0; attempt < k_max_gen_attempts; ++attempt) {
        ++seed_;
        if (puzzle_generate(last_generated_, difficulty_, seed_)) {
            engine_.load_from_game_state(last_generated_);
            last_drawn_moves_ = -1;
            player_dir_       = 0;
            redraw_map();
            refresh_rival_portrait_ui();
            return;
        }
    }

    BN_ASSERT(false, "EndlessState::generate_next: generation failed");
}

void EndlessState::reload_last_generated() {
    engine_.load_from_game_state(last_generated_);
    last_drawn_moves_ = -1;
    player_dir_       = 0;
    redraw_map();
    refresh_rival_portrait_ui();
}

void EndlessState::ensure_map_resources(SharedContext& ctx) {
    (void)ctx;
    if (!bg_) {
        camera_ = bn::camera_ptr::create(0, 0);
        bg_     = bn::regular_bg_items::still_puzzle_map.create_bg(0, 0);
        bg_->set_priority(2);
        bg_->set_camera(*camera_);
        bg_map_ = bg_->map();
    }

    if (!player_sprite_) {
        player_sprite_ = bn::sprite_items::spr_player.create_sprite(0, 0);
        player_sprite_->set_bg_priority(1);
        player_sprite_->set_z_order(1);
        player_sprite_->set_camera(*camera_);
        player_anim_.emplace(bn::create_sprite_animate_action_forever(
            *player_sprite_, 10, bn::sprite_items::spr_player.tiles_item(),
            player_dir_ * 3, player_dir_ * 3, player_dir_ * 3, player_dir_ * 3));
    }
}

void EndlessState::refresh_rival_portrait_ui() {
    if (!ui_manager_) {
        return;
    }

    UIImage* img = ui_manager_->get_image("rival_portrait");
    if (!img) {
        return;
    }

    ui_manager_->change_sprite_image_by_id(img, "mayo_normal");
    img->set_visible(true);
    img->set_horizontal_flip(true);
    img->set_bg_priority(0);
}

void EndlessState::redraw_map() {
    if (bg_map_) {
        render_draw_map(map_cells_, *bg_map_, engine_.data());
    }
}

void EndlessState::update_hud() {
    if (!ui_manager_) {
        return;
    }

    int current_moves = engine_.data().moves;
    if (last_drawn_moves_ == current_moves) {
        return;
    }

    if (auto* node = ui_manager_->get_text("moves_text")) {
        bn::string<24> t = "手 ";
        t.append(bn::to_string<16>(current_moves));
        node->set_text(t);
    }

    if (auto* node = ui_manager_->get_text("stage_text")) {
        bn::string<24> st = "LV";
        st.append(bn::to_string<8>(difficulty_));
        st.append(" #");
        st.append(bn::to_string<8>(score_ + 1));
        node->set_text(st);
    }

    last_drawn_moves_ = current_moves;
}

void EndlessState::get_visual_player_pos(bn::fixed& px, bn::fixed& py) {
    if (puzzle_phase_ == EndlessPuzzlePhase::PP_ANIMATING && move_anim_frames_ < move_anim_max_frames_) {
        bn::fixed t = bn::fixed(move_anim_frames_) / move_anim_max_frames_;
        px = move_src_x_ * 16 + 8 + (move_dst_x_ - move_src_x_) * 16 * t;
        py = move_src_y_ * 16 + 8 + (move_dst_y_ - move_src_y_) * 16 * t;
    } else {
        px = engine_.data().player_x * 16 + 8;
        py = engine_.data().player_y * 16 + 8;
    }
}

void EndlessState::update_camera() {
    if (!camera_) {
        return;
    }

    bn::fixed px, py;
    get_visual_player_pos(px, py);
    camera_->set_position(px - 256, py - 256);
}

void EndlessState::update_player_sprite() {
    if (!player_sprite_) {
        return;
    }

    bn::fixed px, py;
    get_visual_player_pos(px, py);
    player_sprite_->set_position(px - 256, py - 256 - 6);

    if (player_anim_ && !player_anim_->done()) {
        player_anim_->update();
    }
}
