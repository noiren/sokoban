#include "title_state.h"
#include "state_manager.h"
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

TitleState::TitleState(bn::sprite_text_generator& text_gen)
    : text_gen_(text_gen),
      phase_(TitlePhase::EPID_LOGO_DISP),
      step_(PhaseStep::OPENING),
      frame_counter_(0),
      blink_counter_(0),
      ui_manager_(text_gen) {
}

void TitleState::init(StateManager& /*manager*/) {
    ui_manager_.load_screen(ui_data_logo::SCREEN);
    // 最初のフェーズへ（画面はload済みなのでload不要）
    phase_        = TitlePhase::EPID_LOGO_DISP;
    step_         = PhaseStep::OPENING;
    frame_counter_ = 0;
    blink_counter_ = 0;
}

// ==========================================
// フェーズ遷移（カウンタ・step・画面ロードを一元管理）
// ==========================================

void TitleState::go_to_phase(TitlePhase next) {
    phase_        = next;
    step_         = PhaseStep::OPENING;
    frame_counter_ = 0;

    switch (next) {
        case TitlePhase::DOUJIN_NOTICE_DISP:
            ui_manager_.load_screen(ui_data_attention::SCREEN);
            break;
        case TitlePhase::AUTOSAVE_WARN_DISP:
            ui_manager_.load_screen(ui_data_autosave_attension::SCREEN);
            break;
        case TitlePhase::TITLE_DISP:
            ui_manager_.load_screen(ui_data_title::SCREEN);
            blink_counter_ = 0;
            break;
        default:
            break;
    }
}

// ==========================================
// 各フェーズの update
// ==========================================

void TitleState::update_epid_logo(StateManager& /*manager*/) {
    switch (step_) {
        case PhaseStep::OPENING:
            if (!fade_.is_active()) {
                fade_.start_fade_in(FADE_FRAMES);
            }
            if (!fade_.update()) {
                step_ = PhaseStep::RUNNING;
                frame_counter_ = 0;
            }
            break;

        case PhaseStep::RUNNING:
            if (fade_.is_active()) {
                break;
            }
            if (bn::keypad::a_pressed() || frame_counter_ >= EPID_WAIT_FRAMES) {
                step_ = PhaseStep::CLOSING;
                frame_counter_ = 0;
            }
            break;

        case PhaseStep::CLOSING:
            if (!fade_.is_active()) {
                fade_.start_fade_out(FADE_FRAMES);
            }
            if (!fade_.update()) {
                go_to_phase(TitlePhase::DOUJIN_NOTICE_DISP);
            }
            break;
    }
}

void TitleState::update_doujin_notice(StateManager& /*manager*/) {
    switch (step_) {
        case PhaseStep::OPENING:
            if (!fade_.is_active()) {
                fade_.start_fade_in(FADE_FRAMES);
            }
            if (!fade_.update()) {
                step_ = PhaseStep::RUNNING;
                frame_counter_ = 0;
            }
            break;

        case PhaseStep::RUNNING:
            if (bn::keypad::a_pressed() || frame_counter_ >= NOTICE_WAIT_FRAMES) {
                step_ = PhaseStep::CLOSING;
                frame_counter_ = 0;
            }
            break;

        case PhaseStep::CLOSING:
            if (!fade_.is_active()) {
                fade_.start_fade_out(FADE_FRAMES);
            }
            if (!fade_.update()) {
                go_to_phase(TitlePhase::AUTOSAVE_WARN_DISP);
            }
            break;
    }
}

void TitleState::update_autosave_warn(StateManager& /*manager*/) {
    switch (step_) {
        case PhaseStep::OPENING:
            if (!fade_.is_active()) {
                fade_.start_fade_in(FADE_FRAMES);
            }
            if (!fade_.update()) {
                step_ = PhaseStep::RUNNING;
                frame_counter_ = 0;
            }
            break;

        case PhaseStep::RUNNING:
            if (bn::keypad::a_pressed() || frame_counter_ >= AUTOSAVE_WAIT_FRAMES) {
                step_ = PhaseStep::CLOSING;
                frame_counter_ = 0;
            }
            break;

        case PhaseStep::CLOSING:
            if (!fade_.is_active()) {
                fade_.start_fade_out(FADE_FRAMES);
            }
            if (!fade_.update()) {
                go_to_phase(TitlePhase::TITLE_DISP);
            }
            break;
    }
}

void TitleState::update_title(StateManager& manager) {
    switch (step_) {
        case PhaseStep::OPENING:
            if (!fade_.is_active()) {
                fade_.start_fade_in(FADE_FRAMES);
            }
            if (!fade_.update()) {
                step_ = PhaseStep::RUNNING;
                frame_counter_ = 0;
            }
            break;

        case PhaseStep::RUNNING:
            if (bn::keypad::start_pressed() || bn::keypad::a_pressed()) {
                step_ = PhaseStep::CLOSING;
                frame_counter_ = 0;
            }
            break;

        case PhaseStep::CLOSING:
            if (!fade_.is_active()) {
                fade_.start_fade_out(FADE_FRAMES);
            }
            if (!fade_.update()) {
                manager.pop();  // → SaveSelectState へ遷移
            }
            break;
    }
}

// ==========================================
// メインアップデート
// ==========================================

void TitleState::update(StateManager& manager) {
    frame_counter_++;

    switch (phase_) {
        case TitlePhase::EPID_LOGO_DISP:
            update_epid_logo(manager);
            break;
        case TitlePhase::DOUJIN_NOTICE_DISP:
            update_doujin_notice(manager);
            break;
        case TitlePhase::AUTOSAVE_WARN_DISP:
            update_autosave_warn(manager);
            break;
        case TitlePhase::TITLE_DISP:
            update_title(manager);
            break;
    }

    ui_manager_.update();
}

void TitleState::shutdown() {
    FadeEffect::reset_palette();
    ui_manager_.clear_all();
}
