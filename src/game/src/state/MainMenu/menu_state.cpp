#include "state/MainMenu/menu_state.h"
#include "state/Manager/state_manager.h"
#include "input/input_manager.h"
#include "audio/sound_manager.h"
#include "save/game_flags.h"
#include "save/save_data.h"

// ==========================================
// フェーズハンドラテーブルの定義
// （enumの順番通りに関数をマッピング）
// ==========================================
const MenuState::PhaseHandlers MenuState::phase_table_[] = {
    // MAIN
    { &MenuState::enter_main, &MenuState::update_main, &MenuState::exit_main }
};

// ==========================================
// 大枠の State ライフサイクル
// ==========================================
MenuState::MenuState()
    : phase_(MenuPhase::MAIN),
      step_(PhaseStep::OPENING),
      cursor_(0),
      last_selected_(MenuItem::STORY),
      wait_timer_(0) {
    for (int i = 0; i < VISIBLE_MENU_COUNT; ++i) {
        unlocked_flags_[i] = true;
    }
}

void MenuState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    view_.emplace(*ui_manager_); // Viewの生成

    // 最初のフェーズを設定して Enter 関数を呼び出す
    phase_ = MenuPhase::MAIN;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)(ctx);
    }
}

void MenuState::update(StateManager& sm, SharedContext& ctx) {
    // 現在のフェーズの Update 関数を関数ポインタ経由で呼び出す
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    // View（UI）の更新は毎フレーム回す
    if (view_) {
        view_->update();
    }
}

void MenuState::exit(StateManager& /*sm*/, SharedContext& ctx) {
    // 最後のフェーズの Exit 関数を呼ぶ
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)(ctx);
    }

    view_.reset(); // Viewを先に破棄
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}

// ==========================================
// フェーズ遷移
// ==========================================
void MenuState::change_phase(MenuPhase next, SharedContext& ctx) {
    // 1. 現在のフェーズの Exit を呼ぶ
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)(ctx);
    }

    // 2. フェーズ切り替え
    phase_ = next;

    // 3. 新しいフェーズの Enter を呼ぶ
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)(ctx);
    }
}

// ==========================================
// メインメニュー フェーズ
// ==========================================
void MenuState::enter_main(SharedContext& ctx) {
    // SaveSlot のフラグを参照して解禁状況を設定
    bool endless_unlocked   = false;
    bool practice_unlocked  = true;
    bool gallery_unlocked   = true; // ギャラリーは常時（将来フラグ参照も可）
    if (ctx.save) {
        const SaveSlot& slot = ctx.save->slots[ctx.active_slot];
        endless_unlocked  = save_slot_get_flag(slot, FLAG_ENDLESS_UNLOCKED);
        practice_unlocked = save_slot_get_flag(slot, FLAG_PRACTICE_UNLOCKED);
    }

    unlocked_flags_[0] = true;              // STORY
    unlocked_flags_[1] = practice_unlocked; // PRACTICE
    unlocked_flags_[2] = endless_unlocked; // ENDLESS
    unlocked_flags_[3] = gallery_unlocked; // GALLERY
    unlocked_flags_[4] = true;             // SETTINGS

    cursor_ = (int)last_selected_;
    if (cursor_ >= VISIBLE_MENU_COUNT) cursor_ = 0;
    
    // Stateはロジックで判定した「解禁状況配列」と「初期カーソル」をViewに渡すだけ
    view_->init(unlocked_flags_, cursor_);
    
    step_ = PhaseStep::OPENING;
    wait_timer_ = 0;

    // メニュー画面に入ったときにBGMを再生
    SoundManager::instance().play_bgm(BgmId::Mice_dekadence_ummetus, true, 0, true);
}

void MenuState::update_main(StateManager& sm, SharedContext& /*ctx*/) {
    // TitleStateの方式に則り、PhaseStepによる状態遷移を行う
    switch (step_) {
        case PhaseStep::OPENING:
            // メニューが開いた直後の処理（UIの登場演出待ちなどがあればここで待機）
            // 現状は待機なしですぐにRUNNINGへ移行
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING: {
            bool cursor_changed = false;

            auto& inp = InputManager::instance();

            // 入力処理
            if (inp.is_repeat(Action::MoveUp)) {
                cursor_--;
                if (cursor_ < 0) cursor_ = VISIBLE_MENU_COUNT - 1;
                cursor_changed = true;
            }

            if (inp.is_repeat(Action::MoveDown)) {
                cursor_++;
                if (cursor_ >= VISIBLE_MENU_COUNT) cursor_ = 0;
                cursor_changed = true;
            }

            // カーソルが変更された時だけViewへ反映を指示
            if (cursor_changed) {
                view_->update_selection(cursor_);
            }

            // DEBUG用隠しコマンド（SELECT）
            if (inp.is_triggered(Action::OpenMenu)) {
                last_selected_ = MenuItem::DEBUG;
                sm.change_state(StateID::DEBUG_MENU);
                return;
            }

            // Bボタンでタイトルへ戻る
            if (inp.is_triggered(Action::Cancel)) {
                sm.change_state(StateID::TITLE);
                return;
            }

            // Aボタンで決定処理
            if (inp.is_triggered(Action::Decide)) {
                // 未解禁の項目が選ばれた場合は進行させない（ブザー音を鳴らす等）
                if (!unlocked_flags_[cursor_]) {
                    // 例: ctx.sound_manager->play_sfx("buzzer");
                    break;
                }

                // 解禁済みの場合は遷移処理を開始する
                last_selected_ = static_cast<MenuItem>(cursor_);
                
                // View側に遷移時のハケるアニメーションを再生させる
                view_->play_exit_animation();
                
                // アニメーションの完了を待つ時間を設定し、CLOSINGフェーズへ移行
                wait_timer_ = 30; // 30フレーム待機
                step_ = PhaseStep::CLOSING;
            }
            break;
        }

        case PhaseStep::CLOSING:
            // アニメーションの終了を待機
            if (wait_timer_ > 0) {
                wait_timer_--;
            } else {
                // 待機が完了したら実際のState遷移を実行
                switch (last_selected_) {
                    case MenuItem::STORY:    sm.change_state(StateID::STORY); break;
                    case MenuItem::PRACTICE: sm.change_state(StateID::PRACTICE); break;
                    case MenuItem::ENDLESS:  sm.change_state(StateID::ENDLESS); break;
                    case MenuItem::GALLERY:  sm.change_state(StateID::GALLERY); break;
                    case MenuItem::SETTINGS: sm.change_state(StateID::SETTINGS); break;
                    default: break;
                }
            }
            break;
    }
}

void MenuState::exit_main(SharedContext& /*ctx*/) {
    // MAINフェーズ終了時の処理があれば記述
}