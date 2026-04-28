#include "title_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_palettes.h"
#include "bn_string.h"
#include "bn_fixed.h"

// フェード時間（フレーム数）
namespace {
    constexpr int FADE_FRAMES       = 30;  // フェードイン/アウトの長さ
    constexpr int EPID_WAIT_FRAMES  = 90;  // EPIDロゴの表示時間（Aで早送り可）
    constexpr int NOTICE_WAIT_FRAMES = 120; // 注意書きの表示時間
    constexpr int AUTOSAVE_WAIT_FRAMES = 150; // オートセーブ警告の表示時間
}

TitleState::TitleState(bn::sprite_text_generator& text_gen)
    : text_gen_(text_gen),
      phase_(TitlePhase::EPID_LOGO_FADEIN),
      frame_counter_(0),
      blink_counter_(0),
      ui_manager_(text_gen) {
}

void TitleState::init(StateManager& /*manager*/) {
    phase_ = TitlePhase::EPID_LOGO_FADEIN;
    frame_counter_ = 0;
    blink_counter_ = 0;
    clear_sprites();
    ui_manager_.set_bg(BgImageID::LOGO);

    // 黒画面から開始
    bn::bg_palettes::set_fade(bn::color(0, 0, 0), 1);
    bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 1);
}

// ==========================================
// フェードヘルパー
// ==========================================

bool TitleState::fade_in(int duration) {
    bn::fixed fade = bn::fixed(1) - bn::fixed(frame_counter_) / duration;
    if (fade <= 0) {
        fade = 0;
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 0);
        return true;
    }
    bn::bg_palettes::set_fade(bn::color(0, 0, 0), fade);
    bn::sprite_palettes::set_fade(bn::color(0, 0, 0), fade);
    return false;
}

bool TitleState::fade_out(int duration) {
    bn::fixed fade = bn::fixed(frame_counter_) / duration;
    if (fade >= 1) {
        fade = 1;
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), 1);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 1);
        return true;
    }
    bn::bg_palettes::set_fade(bn::color(0, 0, 0), fade);
    bn::sprite_palettes::set_fade(bn::color(0, 0, 0), fade);
    return false;
}

// ==========================================
// 各フェーズの描画
// ==========================================

void TitleState::draw_epid_logo() {
    // UIManagerが背景を管理するため、ここではテキストのみ（必要なら）
    clear_sprites();
}

void TitleState::draw_doujin_notice() {
    clear_sprites();
    text_gen_.set_center_alignment();
    // TODO: 今後画像（注意.png）に差し替える
    text_gen_.generate(0, -24, "この作品は同人作品です", sprites_);
    text_gen_.generate(0,   0, "produced by 黑 - en",    sprites_);
    text_gen_.generate(0,  24, "Press A to continue",      sprites_);
}

void TitleState::draw_autosave_warn() {
    clear_sprites();
    text_gen_.set_center_alignment();
    // TODO: 今後画像（オートセーブ喚起.png）に差し替える
    text_gen_.generate(0, -16, "この作品はオートセーブを使用します",  sprites_);
    text_gen_.generate(0,   0, "セーブデータが上書きされることがあります", sprites_);
    text_gen_.generate(0,  24, "Press A to continue",                        sprites_);
}

void TitleState::draw_title() {
    // 常に背景(TITLE)が出ているため、テキストが不要ならクリアのみ
    clear_sprites();
}

void TitleState::clear_sprites() {
    sprites_.clear();
}

// ==========================================
// メインアップデート
// ==========================================

void TitleState::update(StateManager& manager) {
    frame_counter_++;

    switch (phase_) {

        // ---- EPID GAMES ロゴ ----
        case TitlePhase::EPID_LOGO_FADEIN:
            draw_epid_logo();
            if (fade_in(FADE_FRAMES)) {
                phase_ = TitlePhase::EPID_LOGO_WAIT;
                frame_counter_ = 0;
            }
            break;

        case TitlePhase::EPID_LOGO_WAIT:
            if (bn::keypad::a_pressed() || frame_counter_ >= EPID_WAIT_FRAMES) {
                phase_ = TitlePhase::EPID_LOGO_FADEOUT;
                frame_counter_ = 0;
            }
            break;

        case TitlePhase::EPID_LOGO_FADEOUT:
            if (fade_out(FADE_FRAMES)) {
                clear_sprites();
                ui_manager_.clear_bg();
                phase_ = TitlePhase::DOUJIN_NOTICE_FADEIN;
                frame_counter_ = 0;
            }
            break;

        // ---- 同人作品注意書き ----
        case TitlePhase::DOUJIN_NOTICE_FADEIN:
            draw_doujin_notice();
            if (fade_in(FADE_FRAMES)) {
                phase_ = TitlePhase::DOUJIN_NOTICE_WAIT;
                frame_counter_ = 0;
            }
            break;

        case TitlePhase::DOUJIN_NOTICE_WAIT:
            if (bn::keypad::a_pressed() || frame_counter_ >= NOTICE_WAIT_FRAMES) {
                phase_ = TitlePhase::DOUJIN_NOTICE_FADEOUT;
                frame_counter_ = 0;
            }
            break;

        case TitlePhase::DOUJIN_NOTICE_FADEOUT:
            if (fade_out(FADE_FRAMES)) {
                clear_sprites();
                phase_ = TitlePhase::AUTOSAVE_WARN_FADEIN;
                frame_counter_ = 0;
            }
            break;

        // ---- オートセーブ警告 ----
        case TitlePhase::AUTOSAVE_WARN_FADEIN:
            draw_autosave_warn();
            if (fade_in(FADE_FRAMES)) {
                phase_ = TitlePhase::AUTOSAVE_WARN_WAIT;
                frame_counter_ = 0;
            }
            break;

        case TitlePhase::AUTOSAVE_WARN_WAIT:
            if (bn::keypad::a_pressed() || frame_counter_ >= AUTOSAVE_WAIT_FRAMES) {
                phase_ = TitlePhase::AUTOSAVE_WARN_FADEOUT;
                frame_counter_ = 0;
            }
            break;

        case TitlePhase::AUTOSAVE_WARN_FADEOUT:
            if (fade_out(FADE_FRAMES)) {
                clear_sprites();
                ui_manager_.load_screen(ui_data_title::SCREEN);
                phase_ = TitlePhase::TITLE_FADEIN;
                frame_counter_ = 0;
            }
            break;

        // ---- タイトル画面 ----
        case TitlePhase::TITLE_FADEIN:
            draw_title();
            if (fade_in(FADE_FRAMES)) {
                phase_ = TitlePhase::TITLE_WAIT;
                frame_counter_ = 0;
                blink_counter_ = 0;
            }
            break;

        case TitlePhase::TITLE_WAIT: {
            if (bn::keypad::start_pressed() || bn::keypad::a_pressed()) {
                phase_ = TitlePhase::TITLE_FADEOUT;
                frame_counter_ = 0;
            }
            break;
        }

        case TitlePhase::TITLE_FADEOUT:
            if (fade_out(FADE_FRAMES)) {
                clear_sprites();
                manager.pop();  // → main.cpp で SaveSelectState へ遷移
            }
            break;

        default:
            break;
    }

    ui_manager_.update();
}

void TitleState::shutdown() {
    clear_sprites();
    ui_manager_.clear_bg();
    bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
    bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 0);
}
