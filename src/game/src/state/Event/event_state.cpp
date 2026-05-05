#include "event_state.h"
#include "state/Manager/state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "bn_bg_palettes.h"
#include "ui_data_event.h"
#include "game/story_data.h"
#include "save/save_data.h"

EventState::EventState()
    : script_(nullptr), pc_(0), phase_(EventPhase::EXECUTING),
      wants_puzzle_(false), puzzle_level_(0),
      text_char_index_(0), text_timer_(0), current_text_(nullptr),
      left_char_id_(-1), right_char_id_(-1),
      step_(PhaseStep::OPENING) {
}

void EventState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    if (ctx.story_script_index < NUM_STORY_SCRIPTS) {
        script_ = story_scripts[ctx.story_script_index];
    } else {
        script_ = nullptr;
    }

    pc_              = 0;
    phase_           = EventPhase::EXECUTING;
    wants_puzzle_    = false;
    puzzle_level_    = 0;
    text_char_index_ = 0;
    text_timer_      = 0;
    current_text_    = nullptr;
    left_char_id_    = -1;
    right_char_id_   = -1;

    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_event::SCREEN);
    ui_.emplace(*ui_manager_);
    clear_all();

    bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);

    step_ = PhaseStep::RUNNING;
}

void EventState::update(StateManager& sm, SharedContext& ctx) {
    switch (step_) {
        case PhaseStep::OPENING:
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            update_event(sm, ctx);
            break;

        case PhaseStep::CLOSING:
            break;
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void EventState::update_event(StateManager& sm, SharedContext& ctx) {
    switch (phase_) {
        case EventPhase::EXECUTING:
            execute_next(ctx);
            break;

        case EventPhase::WAITING_INPUT: {
            if (current_text_ != nullptr) {
                int text_len = 0;
                {
                    const char* tmp = current_text_;
                    while (tmp[text_len] != 0) { text_len++; }
                }

                if (text_char_index_ < text_len) {
                    int speed = 1;
                    SaveSlot& save = ctx.save->slots[ctx.active_slot];
                    switch (save.text_speed) {
                        case 0: speed = 3; break;  // slow
                        case 1: speed = 1; break;  // normal
                        case 2: speed = 0; break;  // fast (instant)
                        default: speed = 1; break;
                    }

                    text_timer_++;
                    if (text_timer_ >= speed) {
                        text_timer_ = 0;
                        text_char_index_++;
                        if (speed == 0) text_char_index_ = text_len;
                        update_dialog_text(ctx);
                    }
                }

                if (bn::keypad::a_pressed()) {
                    if (text_char_index_ < text_len) {
                        text_char_index_ = text_len;
                        update_dialog_text(ctx);
                    } else {
                        phase_ = EventPhase::EXECUTING;
                    }
                }
            } else {
                if (bn::keypad::a_pressed()) {
                    phase_ = EventPhase::EXECUTING;
                }
            }
            break;
        }

        case EventPhase::FINISHED:
            if (wants_puzzle_) {
                sm.change_state(StateID::PUZZLE);
            } else {
                ctx.story_script_index++;
                if (ctx.story_script_index < NUM_STORY_SCRIPTS) {
                    sm.change_state(StateID::EVENT);
                } else {
                    sm.change_state(StateID::MENU);
                }
            }
            break;

        default:
            break;
    }
}

void EventState::execute_next(SharedContext& ctx) {
    if (!script_ || pc_ >= script_->num_commands) {
        phase_ = EventPhase::FINISHED;
        return;
    }

    const EventCommand& cmd = script_->commands[pc_];
    pc_++;

    SaveSlot& save = ctx.save->slots[ctx.active_slot];

    switch (cmd.cmd) {
        case EventCmd::TEXT:
            current_text_    = cmd.text;
            text_char_index_ = 0;
            text_timer_      = 0;
            update_dialog_text(ctx);
            phase_ = EventPhase::WAITING_INPUT;
            break;

        case EventCmd::WAIT_INPUT:
            current_text_ = nullptr;
            phase_ = EventPhase::WAITING_INPUT;
            break;

        case EventCmd::CLEAR_TEXT:
            current_text_ = nullptr;
            if (ui_) ui_->set_message("");
            break;

        case EventCmd::SHOW_LEFT:
            left_char_id_ = cmd.arg1;
            if (ui_) ui_->set_left_char(left_char_id_);
            break;

        case EventCmd::SHOW_RIGHT:
            right_char_id_ = cmd.arg1;
            if (ui_) ui_->set_right_char(right_char_id_);
            break;

        case EventCmd::HIDE_LEFT:
            left_char_id_ = -1;
            if (ui_) ui_->clear_left_char();
            break;

        case EventCmd::HIDE_RIGHT:
            right_char_id_ = -1;
            if (ui_) ui_->clear_right_char();
            break;

        case EventCmd::SET_FLAG:
            save_data_set_flag(save, cmd.arg1, true);
            break;

        case EventCmd::CHECK_FLAG:
            if (!save_data_get_flag(save, cmd.arg1)) {
                pc_ += cmd.arg2;  // N個のコマンドをスキップ
            }
            break;

        case EventCmd::GOTO_PUZZLE:
            wants_puzzle_ = true;
            puzzle_level_ = cmd.arg1;
            phase_ = EventPhase::FINISHED;
            break;

        case EventCmd::END:
            phase_ = EventPhase::FINISHED;
            break;

        case EventCmd::PLAY_SE:
            if (ctx.sound) ctx.sound->play_move();  // Placeholder
            break;

        default:
            break;
    }
}

void EventState::update_dialog_text(SharedContext& /*ctx*/) {
    if (!ui_) return;
    if (!current_text_) {
        ui_->set_message("");
        return;
    }

    bn::string<64> partial;
    int count = 0;
    const char* p = current_text_;
    while (*p && count < text_char_index_) {
        partial.append(*p);
        p++;
        count++;
    }

    ui_->set_message(partial);
}

void EventState::clear_all() {
    if (ui_) {
        ui_->clear_chars();
        ui_->set_name("");
        ui_->set_message("");
    }
}

void EventState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    clear_all();
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
    ui_.reset();
    bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
}
