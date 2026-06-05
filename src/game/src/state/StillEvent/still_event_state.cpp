#include "still_event_state.h"
#include "state/Manager/state_manager.h"
#include "state/shared_context.h"
#include "fixdata/fix_data_manager.h"
#include "input/input_manager.h"
#include "bn_sprite_text_generator.h"
#include "bn_keypad.h"
#include "bn_assert.h"
#include "audio/sound_manager.h"
#include "bn_blending.h"
#include "bn_log.h"

const StillEventState::PhaseHandlers StillEventState::phase_table_[] = {
    { &StillEventState::enter_page_enter,  &StillEventState::update_page_enter,  &StillEventState::exit_page_enter  },
    { &StillEventState::enter_msg_fade_in, &StillEventState::update_msg_fade_in, &StillEventState::exit_msg_fade_in },
    { &StillEventState::enter_msg_wait,    &StillEventState::update_msg_wait,    &StillEventState::exit_msg_wait    },
    { &StillEventState::enter_msg_fade_out,&StillEventState::update_msg_fade_out,&StillEventState::exit_msg_fade_out},
    { &StillEventState::enter_page_exit,   &StillEventState::update_page_exit,   &StillEventState::exit_page_exit   },
};

StillEventState::StillEventState()
    : event_entry_(nullptr), page_idx_(0), msg_idx_(0), phase_(StillEventPhase::PAGE_ENTER),
      fade_timer_(0), fade_timer_max_(1) {
}

void StillEventState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    event_entry_ = nullptr;
    for (int i = 0; i < kStillEventCount; ++i) {
        if (bn::string_view(g_still_events[i].id) == ctx.target_event_id) {
            event_entry_ = &g_still_events[i];
            break;
        }
    }
    
    BN_ASSERT(event_entry_ != nullptr, "StillEventState: still event not found");

    page_idx_ = 0;
    msg_idx_  = 0;
    phase_    = StillEventPhase::PAGE_ENTER;

    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void StillEventState::update(StateManager& sm, SharedContext& ctx) {
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }
}

void StillEventState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }
    
    SoundManager::instance().stop_bgm(0);
    text_sprites_.clear();
    still_bg_.reset();
    FadeEffect::reset_palette();
    bn::blending::set_transparency_alpha(1);
}

void StillEventState::change_phase(StillEventPhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }
    phase_ = next;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void StillEventState::enter_page_enter() {
    int duration = 16;
    if (event_entry_ && page_idx_ < event_entry_->page_count) {
        duration = event_entry_->pages[page_idx_].fade_in_frames;
    }
    if (duration <= 0) duration = 1;
    fade_.start_fade_in(duration);
}

void StillEventState::update_page_enter(StateManager& /*sm*/, SharedContext& ctx) {
    if (!fade_.update()) {
        if (event_entry_ && page_idx_ < event_entry_->page_count) {
            apply_current_page(ctx);
            const auto& page = event_entry_->pages[page_idx_];
            if (page.message_count > 0) {
                msg_idx_ = 0;
                apply_current_msg(ctx);
                change_phase(StillEventPhase::MSG_FADE_IN);
            } else {
                change_phase(StillEventPhase::MSG_WAIT);
            }
        } else {
            change_phase(StillEventPhase::PAGE_EXIT);
        }
    }
}

void StillEventState::exit_page_enter() {}

void StillEventState::enter_msg_fade_in() {
    fade_timer_ = 0;
    fade_timer_max_ = MSG_FADE_DURATION;
    set_text_alpha(0);
}

void StillEventState::update_msg_fade_in(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    fade_timer_++;
    set_text_alpha((fade_timer_ * 16) / fade_timer_max_);
    if (fade_timer_ >= fade_timer_max_) {
        set_text_alpha(16);
        change_phase(StillEventPhase::MSG_WAIT);
    }
}

void StillEventState::exit_msg_fade_in() {}

void StillEventState::enter_msg_wait() {
}

void StillEventState::update_msg_wait(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (InputManager::instance().is_triggered(Action::Decide) || bn::keypad::a_pressed() || bn::keypad::b_pressed()) {
        change_phase(StillEventPhase::MSG_FADE_OUT);
    }
}

void StillEventState::exit_msg_wait() {}

void StillEventState::enter_msg_fade_out() {
    fade_timer_ = fade_timer_max_ = MSG_FADE_DURATION;
}

void StillEventState::update_msg_fade_out(StateManager& /*sm*/, SharedContext& ctx) {
    fade_timer_--;
    set_text_alpha((fade_timer_ * 16) / fade_timer_max_);
    if (fade_timer_ <= 0) {
        set_text_alpha(0);
        text_sprites_.clear();
        
        const auto& page = event_entry_->pages[page_idx_];
        msg_idx_++;
        if (msg_idx_ < page.message_count) {
            apply_current_msg(ctx);
            change_phase(StillEventPhase::MSG_FADE_IN);
        } else {
            change_phase(StillEventPhase::PAGE_EXIT);
        }
    }
}

void StillEventState::exit_msg_fade_out() {}

void StillEventState::enter_page_exit() {
    int duration = 16;
    if (event_entry_ && page_idx_ < event_entry_->page_count) {
        duration = event_entry_->pages[page_idx_].fade_out_frames;
    }
    if (duration <= 0) duration = 1;
    fade_.start_fade_out(duration);
}

void StillEventState::update_page_exit(StateManager& sm, SharedContext& ctx) {
    if (!fade_.update()) {
        page_idx_++;
        if (page_idx_ < event_entry_->page_count) {
            still_bg_.reset();
            change_phase(StillEventPhase::PAGE_ENTER);
        } else {
            // 全ページ完了 → 解禁ルール適用
            if (event_entry_ && ctx.save) {
                FixDataManager::instance().apply_unlock_rules(
                    bn::string_view(event_entry_->id),
                    ctx.save->slots[ctx.active_slot],
                    *ctx.save,
                    ctx.active_slot
                );
            }
            if (ctx.event_return_state == StateID::STORY) {
                ctx.story_step_completed = true;
                sm.pop_state();
            } else {
                sm.change_state(ctx.event_return_state);
            }
        }
    }
}

void StillEventState::exit_page_exit() {}

void StillEventState::apply_current_page(SharedContext& /*ctx*/) {
    if (!event_entry_ || page_idx_ >= event_entry_->page_count) return;
    // const auto& page = event_entry_->pages[page_idx_];
    // TODO: Create regular BG from page.still_image_id
}

void StillEventState::apply_current_msg(SharedContext& ctx) {
    if (!event_entry_ || page_idx_ >= event_entry_->page_count) return;
    const auto& page = event_entry_->pages[page_idx_];
    if (msg_idx_ >= page.message_count) return;
    
    const auto& msg = page.messages[msg_idx_];
    
    if (msg.stop_bgm) {
        SoundManager::instance().stop_bgm();
    }
    if (msg.bgm_id != BgmId::COUNT) {
        SoundManager::instance().play_bgm(msg.bgm_id, true);
    }
    if (msg.se_id != SeId::COUNT) {
        SoundManager::instance().play_se(msg.se_id);
    }
    
    refresh_display_text(ctx);
}

void StillEventState::refresh_display_text(SharedContext& ctx) {
    text_sprites_.clear();
    if (!ctx.text_generator || !event_entry_) return;
    const auto& msg = event_entry_->pages[page_idx_].messages[msg_idx_];
    
    ctx.text_generator->generate(0, 56, bn::string_view(msg.text), text_sprites_);
    
    for(auto& sprite : text_sprites_) {
        sprite.set_blending_enabled(true);
    }
    set_text_alpha(0);
}

void StillEventState::set_text_alpha(int alpha) {
    if (text_sprites_.empty()) return;
    bn::fixed a = bn::fixed(alpha) / 16;
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    bn::blending::set_transparency_alpha(a);
}
