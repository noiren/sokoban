#include "still_event_state.h"

#include "state/Manager/state_manager.h"
#include "state/shared_context.h"
#include "fixdata/fix_data_manager.h"
#include "input/input_manager.h"

#include "bn_sprite_text_generator.h"
#include "bn_keypad.h"
#include "bn_assert.h"
#include "audio/sound_manager.h"
#include "animation/sprite_anim_manager.h"

// =============================================================
// UTF-8ヘルパー（EventStateと同じロジック）
// =============================================================
namespace {

int get_utf8_char_count(bn::string_view str) {
    int count = 0;
    for (int i = 0; i < str.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if ((c & 0xC0) != 0x80) count++;
    }
    return count;
}

int get_utf8_byte_index(bn::string_view str, int char_limit) {
    int byte_idx = 0, char_count = 0;
    while (byte_idx < str.size() && char_count < char_limit) {
        unsigned char c = static_cast<unsigned char>(str[byte_idx]);
        if ((c & 0xC0) != 0x80) {
            char_count++;
            if (char_count > char_limit) break;
        }
        byte_idx++;
    }
    while (byte_idx < str.size()) {
        unsigned char c = static_cast<unsigned char>(str[byte_idx]);
        if ((c & 0xC0) != 0x80) break;
        byte_idx++;
    }
    return byte_idx;
}

} // namespace

// =============================================================
// フェーズテーブル
// =============================================================
const StillEventState::PhaseHandlers StillEventState::phase_table_[] = {
    { &StillEventState::enter_fade_in,  &StillEventState::update_fade_in,  &StillEventState::exit_fade_in  },
    { &StillEventState::enter_showing,  &StillEventState::update_showing,  &StillEventState::exit_showing  },
    { &StillEventState::enter_fade_out, &StillEventState::update_fade_out, &StillEventState::exit_fade_out },
};

// =============================================================
// コンストラクタ
// =============================================================
StillEventState::StillEventState()
    : event_entry_(nullptr), pc_(0), phase_(StillEventPhase::FADE_IN),
      step_(PhaseStep::OPENING), typewrite_text_(), displayed_chars_(0),
      text_timer_(0), skip_requested_(false) {
}

// =============================================================
// State ライフサイクル
// =============================================================
void StillEventState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    event_entry_ = FixDataManager::instance().find_event(ctx.target_event_id);
    BN_ASSERT(event_entry_ != nullptr, "StillEventState: event not found");

    pc_              = 0;
    skip_requested_  = false;
    displayed_chars_ = 0;
    text_timer_      = TYPING_SPEED_NORMAL;
    phase_           = StillEventPhase::FADE_IN;
    step_            = PhaseStep::OPENING;

    // TODO: スチル画像のロード
    // still_bg_ = bn::regular_bg_items::【スチルID】.create_bg(0, 0);

    // 最初の行を適用
    if (event_entry_ && event_entry_->line_count > 0) {
        apply_line(0, ctx);
    }

    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void StillEventState::update(StateManager& sm, SharedContext& ctx) {
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }
    
    SpriteAnimManager::instance().update();
}

void StillEventState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }
    
    // イベント終了時は必ず再生中のBGMを止める
    SoundManager::instance().stop_bgm(0);
    
    SpriteAnimManager::instance().stop(emotion_handle_);
    emotion_handle_ = INVALID_ANIM_HANDLE;
    text_sprites_.clear();
    still_bg_.reset();
    FadeEffect::reset_palette();
}

// =============================================================
// フェーズ遷移
// =============================================================
void StillEventState::change_phase(StillEventPhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }
    phase_ = next;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

// =============================================================
// FADE_IN フェーズ
// =============================================================
void StillEventState::enter_fade_in() {
    fade_.start_fade_in(FADE_DURATION);
    step_ = PhaseStep::RUNNING;
}

void StillEventState::update_fade_in(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (!fade_.update()) {
        // フェードイン完了 → テキスト表示へ
        change_phase(StillEventPhase::SHOWING);
    }
}

void StillEventState::exit_fade_in() {}

// =============================================================
// SHOWING フェーズ（テキスト送り）
// =============================================================
void StillEventState::enter_showing() {
    skip_requested_  = false;
    displayed_chars_ = 0;
    text_timer_      = TYPING_SPEED_NORMAL;
    step_ = PhaseStep::RUNNING;
}

void StillEventState::update_showing(StateManager& sm, SharedContext& ctx) {
    // B長押しで即時全表示
    if (bn::keypad::b_held()) {
        displayed_chars_ = get_utf8_char_count(typewrite_text_);
        refresh_display_text(ctx);
        return;
    }

    // B単押し / A長押しで現在行を即時全表示
    if (bn::keypad::b_pressed()) {
        skip_requested_ = true;
    }

    if (skip_requested_) {
        displayed_chars_ = get_utf8_char_count(typewrite_text_);
        refresh_display_text(ctx);
        skip_requested_ = false;
        return;
    }

    // タイプライター更新
    if (displayed_chars_ < get_utf8_char_count(typewrite_text_)) {
        const int speed = bn::keypad::a_held() ? TYPING_SPEED_FAST : TYPING_SPEED_NORMAL;
        text_timer_--;
        if (text_timer_ <= 0) {
            text_timer_ = speed;
            advance_typewriter(1);
            refresh_display_text(ctx);
        }
        return;
    }

    // 全文字表示済み → Aボタン入力待ち
    if (InputManager::instance().is_triggered(Action::Decide)) {
        // 次の行へ
        pc_++;
        if (!event_entry_ || pc_ >= event_entry_->line_count) {
            // 全行終了 → フェードアウト
            change_phase(StillEventPhase::FADE_OUT);
        } else {
            apply_line(pc_, ctx);
            enter_showing(); // 次の行のタイプライターを初期化
        }
    }
}

void StillEventState::exit_showing() {}

// =============================================================
// FADE_OUT フェーズ
// =============================================================
void StillEventState::enter_fade_out() {
    fade_.start_fade_out(FADE_DURATION);
    step_ = PhaseStep::RUNNING;
}

void StillEventState::update_fade_out(StateManager& sm, SharedContext& ctx) {
    if (!fade_.update()) {
        // フェードアウト完了 → StoryStateに完了を通知してpop
        ctx.story_step_completed = true;
        sm.pop_state();
    }
}

void StillEventState::exit_fade_out() {}

// =============================================================
// 内部ヘルパー
// =============================================================
void StillEventState::apply_line(int line_index, SharedContext& /*ctx*/) {
    if (!event_entry_) return;
    if (line_index < 0 || line_index >= event_entry_->line_count) return;

    const FdEventLine& line = event_entry_->lines[line_index];
    
    // オーディオ再生
    if (line.stop_bgm) {
        SoundManager::instance().stop_bgm();
    }
    if (line.bgm_id != BgmId::COUNT) {
        SoundManager::instance().play_bgm(line.bgm_id, true);
    }
    if (line.se_id != SeId::COUNT) {
        SoundManager::instance().play_se(line.se_id);
    }
    
    typewrite_text_  = bn::string_view(line.text);
    displayed_chars_ = 0;
    text_timer_      = TYPING_SPEED_NORMAL;

    // エモーションの表示
    SpriteAnimManager::instance().stop(emotion_handle_);
    emotion_handle_ = INVALID_ANIM_HANDLE;
    if (line.emotion_id && line.emotion_id[0] != '\0') {
        for (int i = 0; i < static_cast<int>(SpriteAnimId::COUNT); ++i) {
            if (bn::string_view(g_sprite_anims[i].id) == bn::string_view(line.emotion_id)) {
                int ex = (line.position == FdPosition::Left) ? -56 : 64;
                int ey = -48;
                emotion_handle_ = SpriteAnimManager::instance().play(
                    static_cast<SpriteAnimId>(i), ex, ey, -1
                );
                SpriteAnimManager::instance().set_bg_priority(emotion_handle_, 0);
                break;
            }
        }
    }

    // テキストスプライトをクリア（次のrefresh_display_textで再生成）
    text_sprites_.clear();
}

void StillEventState::advance_typewriter(int chars_per_frame) {
    displayed_chars_ += chars_per_frame;
    int max_chars = get_utf8_char_count(typewrite_text_);
    
    // 文字送り音（ポポポ）の再生
    if (chars_per_frame > 0 && displayed_chars_ <= max_chars) {
        SoundManager::instance().play_se(SeId::Default_Popopo);
    }
    
    if (displayed_chars_ > max_chars) displayed_chars_ = max_chars;
}

void StillEventState::refresh_display_text(SharedContext& ctx) {
    text_sprites_.clear();
    if (!ctx.text_generator || typewrite_text_.empty()) return;

    int bytes = get_utf8_byte_index(typewrite_text_, displayed_chars_);
    bn::string_view partial(typewrite_text_.data(), (unsigned)bytes);

    // 画面下部（y=56付近 = GBAの160px中 下1/4）にテキスト表示
    ctx.text_generator->generate(0, 56, partial, text_sprites_);
}
