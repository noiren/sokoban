#include "puzzle_state.h"
#include "state/Manager/state_manager.h"
#include "state/state_id.h"
#include "input/input_manager.h"
#include "graphics/renderer.h"
#include "ui_data_sokoban_main.h"
#include "bn_regular_bg_items_still_puzzle_map.h"
#include "bn_string.h"
#include "bn_keypad.h"
#include "bn_assert.h"
#include "game/levels.h"
#include "save/user_data_puzzle.h"
#include "bn_sprite_items_spr_player.h"

// =============================================================================
// フェーズテーブルの実体定義（順番は PuzzlePhase enum と一致させること）
// =============================================================================
const PuzzleState::PhaseHandlers PuzzleState::phase_table_[(int)PuzzlePhase::COUNT] = {
    /* PLAYING   */ { &PuzzleState::enter_playing,   &PuzzleState::update_playing,   &PuzzleState::exit_playing   },
    /* ANIMATING */ { &PuzzleState::enter_animating, &PuzzleState::update_animating, &PuzzleState::exit_animating },
    /* FAILED    */ { &PuzzleState::enter_failed,    &PuzzleState::update_failed,    &PuzzleState::exit_failed    },
    /* CLEARED   */ { &PuzzleState::enter_cleared,   &PuzzleState::update_cleared,   &PuzzleState::exit_cleared   },
};

// =============================================================================
// コンストラクタ
// =============================================================================
PuzzleState::PuzzleState()
    : phase_(PuzzlePhase::PLAYING),
      last_result_(PuzzleEngine::Result::CONTINUE),
      current_level_(0),
      last_drawn_moves_(-1),
      current_event_index_(0),
      anim_frame_(0),
      frame_counter_(0) {
}

// =============================================================================
// State ライフサイクル
// =============================================================================
void PuzzleState::enter(StateManager& sm, SharedContext& ctx) {
    BN_ASSERT(ctx.text_generator != nullptr, "PuzzleState::enter: text_generator is null");
    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_sokoban_main::SCREEN);

    current_level_    = ctx.target_puzzle_level;
    last_drawn_moves_ = -1;

    level_init();

    phase_ = PuzzlePhase::PLAYING;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }

    // イントロイベントがあれば再生
    if (level_intro_event[current_level_] != nullptr && !ctx.puzzle_played_intro[current_level_]) {
        ctx.target_event_id = level_intro_event[current_level_];
        ctx.use_puzzle_event_table = true;
        ctx.event_is_overlay = true;
        ctx.event_return_state = StateID::PUZZLE;
        ctx.puzzle_played_intro[current_level_] = true;
        sm.push_state(StateID::EVENT);
        return;
    }

}

void PuzzleState::update(StateManager& sm, SharedContext& ctx) {
    update_hud();

    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    // プレイヤーが移動した後にカメラとスプライトを更新することでちらつきを防ぐ
    update_camera();
    update_player_sprite();

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void PuzzleState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
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

// =============================================================================
// フェーズ遷移
// =============================================================================
void PuzzleState::change_phase(PuzzlePhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }
    phase_ = next;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

// =============================================================================
// フェーズ：PLAYING（入力受付・通常プレイ）
// =============================================================================
void PuzzleState::enter_playing() {
    // 失敗メッセージが表示されていれば消す
    if (ui_manager_) {
        if (auto* node = ui_manager_->get_text("start_text")) {
            node->set_visible(false);
        }
    }
}

void PuzzleState::update_playing(StateManager& sm, SharedContext& ctx) {
    // チュートリアルトリガーは削除されました

    // PLAYING フェーズ中はフレームカウンタを進める
    ++frame_counter_;

    // Bボタン：メニューへ戻る
    if (InputManager::instance().is_triggered(Action::Cancel)) {
        sm.change_state(ctx.puzzle_return_state);
        return;
    }

    // SELECT / R / START：現在のレベルをリセット
    if (bn::keypad::select_pressed() || bn::keypad::r_pressed() || bn::keypad::start_pressed()) {
        level_init();
        change_phase(PuzzlePhase::PLAYING);
        return;
    }

    // L：Undo (1手戻る)
    if (bn::keypad::l_pressed()) {
        if (engine_.try_undo()) {
            redraw_map();
            return;
        }
    }

    int dx = 0, dy = 0;
    if      (InputManager::instance().is_triggered(Action::MoveUp))    dy = -1;
    else if (InputManager::instance().is_triggered(Action::MoveDown))  dy =  1;
    else if (InputManager::instance().is_triggered(Action::MoveLeft))  dx = -1;
    else if (InputManager::instance().is_triggered(Action::MoveRight)) dx =  1;

    if (dx == 0 && dy == 0) return;

    int old_px = engine_.data().player_x;
    int old_py = engine_.data().player_y;

    // ロジックを1手進める（このフレーム内で連鎖計算まで全て完了）
    last_result_ = engine_.try_move(dx, dy);

    // アニメーション用の方向更新
    if (dx < 0) player_dir_ = 2; // Left
    if (dx > 0) player_dir_ = 3; // Right
    if (dy < 0) player_dir_ = 1; // Up
    if (dy > 0) player_dir_ = 0; // Down

    if (engine_.data().player_x != old_px || engine_.data().player_y != old_py) {
        // 移動が発生した場合はアニメーションフェーズへ
        move_src_x_ = old_px;
        move_src_y_ = old_py;
        move_dst_x_ = engine_.data().player_x;
        move_dst_y_ = engine_.data().player_y;
        move_anim_frames_ = 0;
        
        // 歩行アニメーション開始
        if (player_sprite_) {
            player_anim_.emplace(bn::create_sprite_animate_action_forever(
                *player_sprite_, 3, bn::sprite_items::spr_player.tiles_item(),
                player_dir_ * 3 + 1, player_dir_ * 3 + 0, player_dir_ * 3 + 2, player_dir_ * 3 + 0));
        }

        change_phase(PuzzlePhase::ANIMATING);
    } else if (!engine_.events().empty()) {
        // イベントがあればアニメーションフェーズへ
        change_phase(PuzzlePhase::ANIMATING);
    } else {
        // 移動できなかった場合でも念のため結果を確認
        if (last_result_ == PuzzleEngine::Result::FAILED)  change_phase(PuzzlePhase::FAILED);
        if (last_result_ == PuzzleEngine::Result::CLEARED) change_phase(PuzzlePhase::CLEARED);
    }
}

void PuzzleState::exit_playing() {}

// =============================================================================
// フェーズ：ANIMATING（イベントキューの消費・演出再生）
// =============================================================================
void PuzzleState::enter_animating() {
    current_event_index_ = 0;
    anim_frame_ = 0;
    // アニメーション開始前にマップを再描画（暫定：即時反映）
    // TODO: イベントを1つずつ消費してアニメーションする実装に置き換える
    redraw_map();
}

void PuzzleState::update_animating(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    // ANIMATING フェーズ中もフレームカウンタを進める
    ++frame_counter_;

    if (move_anim_frames_ < move_anim_max_frames_) {
        move_anim_frames_++;
        return; // アニメーション中はここで待機
    }

    const auto& events = engine_.events();

    // 暫定実装：全イベントを1フレームで処理（移動アニメーション完了後に適用）
    current_event_index_ = events.size();

    if (current_event_index_ >= events.size()) {
        engine_.clear_events();

        // ロジック結果に応じてフェーズ遷移
        if (last_result_ == PuzzleEngine::Result::FAILED) {
            change_phase(PuzzlePhase::FAILED);
        } else if (last_result_ == PuzzleEngine::Result::CLEARED) {
            change_phase(PuzzlePhase::CLEARED);
        } else {
            // アニメーションを待機状態に戻す
            if (player_sprite_) {
                player_anim_.emplace(bn::create_sprite_animate_action_forever(
                    *player_sprite_, 10, bn::sprite_items::spr_player.tiles_item(),
                    player_dir_ * 3, player_dir_ * 3, player_dir_ * 3, player_dir_ * 3));
            }
            change_phase(PuzzlePhase::PLAYING);
        }
    }
}

void PuzzleState::exit_animating() {}

// =============================================================================
// フェーズ：FAILED（落下ミス）
// =============================================================================
void PuzzleState::enter_failed() {
    if (ui_manager_) {
        if (auto* node = ui_manager_->get_text("start_text")) {
            node->set_text("FAILED! SELECT/R: RETRY");
            node->set_visible(true);
        }
    }
}

void PuzzleState::update_failed(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (bn::keypad::select_pressed() || bn::keypad::r_pressed()) {
        level_init();
        change_phase(PuzzlePhase::PLAYING);
    }
}

void PuzzleState::exit_failed() {
    if (ui_manager_) {
        if (auto* node = ui_manager_->get_text("start_text")) {
            node->set_visible(false);
        }
    }
}

// =============================================================================
// フェーズ：CLEARED（クリア）
// =============================================================================
void PuzzleState::enter_cleared() {
    if (ui_manager_) {
        if (auto* node = ui_manager_->get_text("clear_text")) {
            // やり込み：樽を一度も落とさなければ PERFECT 表示
            if (engine_.data().dropped_barrels == 0) {
                node->set_text("PERFECT CLEAR!");
            } else {
                node->set_text("CLEAR!");
            }
            node->set_visible(true);
        }
        if (auto* node = ui_manager_->get_text("start_text")) {
            node->set_text("PRESS A/START: NEXT");
            node->set_visible(true);
        }
    }
}

void PuzzleState::update_cleared(StateManager& sm, SharedContext& ctx) {
    if (InputManager::instance().is_triggered(Action::Decide) ||
        bn::keypad::start_pressed()) {

        // アウトロイベントがあれば再生
        if (level_outro_event[current_level_] != nullptr && !ctx.puzzle_played_outro[current_level_]) {
            ctx.target_event_id = level_outro_event[current_level_];
            ctx.use_puzzle_event_table = true;
            ctx.event_is_overlay = true;
            ctx.event_return_state = StateID::PUZZLE;
            ctx.puzzle_played_outro[current_level_] = true;
            sm.push_state(StateID::EVENT);
            return;
        }

        // ストーリーモードから呼ばれた場合は pop_state() で StoryState の resume() を起動
        if (ctx.puzzle_return_state == StateID::STORY) {
            // ストーリークリア記録を UserDataPuzzle に保存
            UserDataPuzzle puzzle_data;
            user_data_puzzle_load(puzzle_data);
            user_data_puzzle_set_story_cleared(puzzle_data, current_level_);
            user_data_puzzle_save(puzzle_data);

            ctx.story_step_completed = true;
            sm.pop_state();
        } else if (ctx.puzzle_return_state == StateID::PRACTICE) {
            // プラクティスモード：クリア結果を ctx に乗せて PRACTICE へ戻る
            ctx.puzzle_just_cleared  = true;
            ctx.puzzle_clear_level   = current_level_;
            ctx.puzzle_clear_moves   = engine_.data().moves;
            ctx.puzzle_clear_frames  = frame_counter_;
            sm.change_state(StateID::PRACTICE);
        } else {
            // 非ストーリーモード：従来通り次のレベルへ or リターン
            int next_level = current_level_ + 1;
            if (next_level < get_num_levels()) {
                ctx.target_puzzle_level = next_level;
                sm.change_state(StateID::PUZZLE);
            } else {
                sm.change_state(ctx.puzzle_return_state);
            }
        }
    }
}

void PuzzleState::exit_cleared() {}

// =============================================================================
// 内部ヘルパー
// =============================================================================
void PuzzleState::level_init() {
    engine_.load_level(current_level_);
    last_drawn_moves_ = -1;
    frame_counter_    = 0;  // フレームカウンタをリセット
    player_dir_       = 0;  // 向きをリセット
    player_dir_       = 0;  // 向きをリセット

    if (!bg_) {
        // camera を先に作成
        camera_ = bn::camera_ptr::create(0, 0);

        bg_     = bn::regular_bg_items::still_puzzle_map.create_bg(0, 0);
        bg_->set_priority(2);
        bg_->set_camera(*camera_); // camera を BG にアタッチ
        bg_map_ = bg_->map();
    }

    if (!player_sprite_) {
        player_sprite_ = bn::sprite_items::spr_player.create_sprite(0, 0);
        player_sprite_->set_bg_priority(1);
        player_sprite_->set_z_order(1);
        player_sprite_->set_camera(*camera_); // カメラをスプライトにもアタッチ
        player_anim_.emplace(bn::create_sprite_animate_action_forever(
            *player_sprite_, 10, bn::sprite_items::spr_player.tiles_item(),
            player_dir_ * 3, player_dir_ * 3, player_dir_ * 3, player_dir_ * 3));
    }

    redraw_map();
}

void PuzzleState::redraw_map() {
    if (bg_map_) {
        render_draw_map(map_cells_, *bg_map_, engine_.data());
    }
}

void PuzzleState::update_hud() {
    if (!ui_manager_) return;

    int current_moves = engine_.data().moves;
    if (last_drawn_moves_ == current_moves) return; // 変化がなければ再描画しない

    if (auto* node = ui_manager_->get_text("moves_text")) {
        node->set_text(bn::to_string<16>(current_moves));
    }
    last_drawn_moves_ = current_moves;
}

void PuzzleState::get_visual_player_pos(bn::fixed& px, bn::fixed& py) {
    if (phase_ == PuzzlePhase::ANIMATING && move_anim_frames_ < move_anim_max_frames_) {
        bn::fixed t = bn::fixed(move_anim_frames_) / move_anim_max_frames_;
        px = move_src_x_ * 16 + 8 + (move_dst_x_ - move_src_x_) * 16 * t;
        py = move_src_y_ * 16 + 8 + (move_dst_y_ - move_src_y_) * 16 * t;
    } else {
        px = engine_.data().player_x * 16 + 8;
        py = engine_.data().player_y * 16 + 8;
    }
}

void PuzzleState::update_camera() {
    if (!camera_) return;

    bn::fixed px, py;
    get_visual_player_pos(px, py);

    camera_->set_position(px - 256, py - 256);
}

void PuzzleState::update_player_sprite() {
    if (!player_sprite_) return;

    bn::fixed px, py;
    get_visual_player_pos(px, py);

    // スプライトは少しだけ上にずらして足元を合わせる（16x32サイズ想定）
    player_sprite_->set_position(px - 256, py - 256 - 6);
    
    if (player_anim_ && !player_anim_->done()) {
        player_anim_->update();
    }
}
