#include "title_state.h"
#include "state/Manager/state_manager.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_palettes.h"
#include "bn_string.h"
#include "ui_data_logo.h"
#include "ui_data_attention.h"
#include "ui_data_autosave_attension.h"
#include "ui_data_title.h"

namespace {
    constexpr int FADE_FRAMES          = 30;  // フェードイン/アウトの長さ
    constexpr int EPID_WAIT_FRAMES     = 90;  // EPIDロゴの表示時間（Aで早送り可）
    constexpr int NOTICE_WAIT_FRAMES   = 120; // 注意書きの表示時間
    constexpr int AUTOSAVE_WAIT_FRAMES = 150; // オートセーブ警告の表示時間
}

// ==========================================
// フェーズハンドラテーブルの定義
// （enumの順番通りに関数をマッピング）
// ==========================================
const TitleState::PhaseHandlers TitleState::phase_table_[] = {
    // EPID_LOGO_DISP
    { &TitleState::enter_epid_logo, &TitleState::update_epid_logo, &TitleState::exit_epid_logo },
    // DOUJIN_NOTICE_DISP
    { &TitleState::enter_doujin_notice, &TitleState::update_doujin_notice, &TitleState::exit_doujin_notice },
    // AUTOSAVE_WARN_DISP
    { &TitleState::enter_autosave_warn, &TitleState::update_autosave_warn, &TitleState::exit_autosave_warn },
    // TITLE_DISP
    { &TitleState::enter_title, &TitleState::update_title, &TitleState::exit_title }
};

// ==========================================
// 大枠の State ライフサイクル
// ==========================================

TitleState::TitleState()
    : phase_(TitlePhase::EPID_LOGO_DISP),
      step_(PhaseStep::OPENING),
      frame_counter_(0),
      blink_counter_(0) {
}

void TitleState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    
    // 最初のフェーズを設定して Enter 関数を呼び出す
    phase_ = TitlePhase::EPID_LOGO_DISP;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void TitleState::update(StateManager& sm, SharedContext& ctx) {
    frame_counter_++;

    // 現在のフェーズの Update 関数を関数ポインタ経由で呼び出す（switch文は不要！）
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void TitleState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    // 最後のフェーズの Exit 関数を呼ぶ
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    FadeEffect::reset_palette();
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}

// ==========================================
// フェーズ遷移
// ==========================================

void TitleState::change_phase(TitlePhase next) {
    // 1. 現在のフェーズの Exit を呼ぶ
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    // 2. フェーズ切り替えと共通変数のリセット
    phase_ = next;
    frame_counter_ = 0;

    // 3. 新しいフェーズの Enter を呼ぶ
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

// ==========================================
// 1. EPIDロゴ
// ==========================================
void TitleState::enter_epid_logo() {
    ui_manager_->load_screen(ui_data_logo::SCREEN);
    step_ = PhaseStep::OPENING;
    fade_.start_fade_in(FADE_FRAMES); // フェード開始処理を Enter に集約
}

void TitleState::update_epid_logo(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    switch (step_) {
        case PhaseStep::OPENING:
            if (!fade_.update()) {
                step_ = PhaseStep::RUNNING;
                frame_counter_ = 0;
            }
            break;
        case PhaseStep::RUNNING:
            if (bn::keypad::a_pressed() || frame_counter_ >= EPID_WAIT_FRAMES) {
                step_ = PhaseStep::CLOSING;
                fade_.start_fade_out(FADE_FRAMES); // 次のステップの準備
            }
            break;
        case PhaseStep::CLOSING:
            if (!fade_.update()) {
                change_phase(TitlePhase::DOUJIN_NOTICE_DISP);
            }
            break;
    }
}

void TitleState::exit_epid_logo() {} // 特になければ空でOK

// ==========================================
// 2. 同人注意書き
// ==========================================
void TitleState::enter_doujin_notice() {
    ui_manager_->load_screen(ui_data_attention::SCREEN);
    step_ = PhaseStep::OPENING;
    fade_.start_fade_in(FADE_FRAMES);
}

void TitleState::update_doujin_notice(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    switch (step_) {
        case PhaseStep::OPENING:
            if (!fade_.update()) {
                step_ = PhaseStep::RUNNING;
                frame_counter_ = 0;
            }
            break;
        case PhaseStep::RUNNING:
            if (bn::keypad::a_pressed() || frame_counter_ >= NOTICE_WAIT_FRAMES) {
                step_ = PhaseStep::CLOSING;
                fade_.start_fade_out(FADE_FRAMES);
            }
            break;
        case PhaseStep::CLOSING:
            if (!fade_.update()) {
                change_phase(TitlePhase::AUTOSAVE_WARN_DISP);
            }
            break;
    }
}

void TitleState::exit_doujin_notice() {}

// ==========================================
// 3. オートセーブ警告
// ==========================================
void TitleState::enter_autosave_warn() {
    ui_manager_->load_screen(ui_data_autosave_attension::SCREEN);
    step_ = PhaseStep::OPENING;
    fade_.start_fade_in(FADE_FRAMES);
}

void TitleState::update_autosave_warn(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    switch (step_) {
        case PhaseStep::OPENING:
            if (!fade_.update()) {
                step_ = PhaseStep::RUNNING;
                frame_counter_ = 0;
            }
            break;
        case PhaseStep::RUNNING:
            if (bn::keypad::a_pressed() || frame_counter_ >= AUTOSAVE_WAIT_FRAMES) {
                step_ = PhaseStep::CLOSING;
                fade_.start_fade_out(FADE_FRAMES);
            }
            break;
        case PhaseStep::CLOSING:
            if (!fade_.update()) {
                change_phase(TitlePhase::TITLE_DISP);
            }
            break;
    }
}

void TitleState::exit_autosave_warn() {}

// ==========================================
// 4. タイトル表示
// ==========================================
void TitleState::enter_title() {
    ui_manager_->load_screen(ui_data_title::SCREEN);
    step_ = PhaseStep::OPENING;
    blink_counter_ = 0;
    fade_.start_fade_in(FADE_FRAMES);
}

void TitleState::update_title(StateManager& sm, SharedContext& /*ctx*/) {
    switch (step_) {
        case PhaseStep::OPENING:
            if (!fade_.update()) {
                step_ = PhaseStep::RUNNING;
                frame_counter_ = 0;
            }
            break;
        case PhaseStep::RUNNING:
            if (bn::keypad::start_pressed() || bn::keypad::a_pressed()) {
                step_ = PhaseStep::CLOSING;
                fade_.start_fade_out(FADE_FRAMES);
            }
            break;
        case PhaseStep::CLOSING:
            if (!fade_.update()) {
                // 画面遷移
                sm.change_state(StateID::SAVE_SELECT);
            }
            break;
    }
}

void TitleState::exit_title() {}