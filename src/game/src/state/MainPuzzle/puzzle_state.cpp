#include "puzzle_state.h"
#include "state/Manager/state_manager.h"
#include "input/input_manager.h"
#include "graphics/renderer.h"
#include "ui_data_sokoban_main.h"
#include "bn_regular_bg_items_still_puzzle_map.h"
#include "bn_string.h"
#include "bn_keypad.h"
#include "game/sokoban.h"

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
      anim_frame_(0) {
}

// =============================================================================
// State ライフサイクル
// =============================================================================
void PuzzleState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_sokoban_main::SCREEN);

    current_level_    = ctx.target_puzzle_level;
    last_drawn_moves_ = -1;

    level_init();

    phase_ = PuzzlePhase::PLAYING;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void PuzzleState::update(StateManager& sm, SharedContext& ctx) {
    update_hud();

    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    // プレイヤーが移動した後にカメラを更新することでちらつきを防ぐ
    update_camera();

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
    // Bボタン：メニューへ戻る
    if (InputManager::instance().is_triggered(Action::Cancel)) {
        sm.change_state(ctx.puzzle_return_state);
        return;
    }

    // SELECT / R：現在のレベルをリセット
    if (bn::keypad::select_pressed() || bn::keypad::r_pressed()) {
        level_init();
        change_phase(PuzzlePhase::PLAYING);
        return;
    }

    int dx = 0, dy = 0;
    if      (InputManager::instance().is_triggered(Action::MoveUp))    dy = -1;
    else if (InputManager::instance().is_triggered(Action::MoveDown))  dy =  1;
    else if (InputManager::instance().is_triggered(Action::MoveLeft))  dx = -1;
    else if (InputManager::instance().is_triggered(Action::MoveRight)) dx =  1;

    if (dx == 0 && dy == 0) return;

    // ロジックを1手進める（このフレーム内で連鎖計算まで全て完了）
    last_result_ = engine_.try_move(dx, dy);

    if (!engine_.events().empty()) {
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
    const auto& events = engine_.events();

    // 暫定実装：全イベントを1フレームで処理（アニメーションなし）
    // TODO: イベントを1つずつ消費し、MOVE_FGは補間移動、CHANGE_BGはBG書き換えにする
    current_event_index_ = events.size();

    if (current_event_index_ >= events.size()) {
        engine_.clear_events();

        // ロジック結果に応じてフェーズ遷移
        if (last_result_ == PuzzleEngine::Result::FAILED) {
            change_phase(PuzzlePhase::FAILED);
        } else if (last_result_ == PuzzleEngine::Result::CLEARED) {
            change_phase(PuzzlePhase::CLEARED);
        } else {
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
        int next_level = current_level_ + 1;
        if (next_level < get_num_levels()) {
            ctx.target_puzzle_level = next_level;
            sm.change_state(StateID::PUZZLE);
        } else {
            sm.change_state(ctx.puzzle_return_state);
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

    if (!bg_) {
        // camera を先に作成
        camera_ = bn::camera_ptr::create(0, 0);

        bg_     = bn::regular_bg_items::still_puzzle_map.create_bg(0, 0);
        bg_->set_priority(2);
        bg_->set_camera(*camera_); // camera を BG にアタッチ
        bg_map_ = bg_->map();
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

void PuzzleState::update_camera() {
    if (!camera_) return;

    // Butano bgs_manager ソースより:
    //   hw_x = -(bg_pos - cam_pos) - screen_w/2 + half_dims.width()
    //        = cam_pos + 8    (bg_pos=0, screen_w=240, half_dims=128)
    //   hw_y = cam_pos_y + 48 (screen_h=160, half_dims=128)
    //
    // プレイヤー(BG pixel px)を画面中心(hardware x=120)に置くには:
    //   BGHOFS = px - 120  →  cam_pos + 8 = px - 120  →  cam_pos = px - 128
    // Y も同様: BGVOFS = py - 80  →  cam_pos + 48 = py - 80  →  cam_pos = py - 128
    bn::fixed px = engine_.data().player_x * 8 + 4;
    bn::fixed py = engine_.data().player_y * 8 + 4;

    camera_->set_position(px - 128, py - 128);
}
