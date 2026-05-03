#include "event_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "bn_bg_palettes.h"
#include "ui_data_event.h"

EventState::EventState(bn::sprite_text_generator& text_gen, SoundManager& sound, SaveSlot& save)
    : text_gen_(text_gen), sound_(sound), save_(save),
      script_(nullptr), pc_(0), phase_(EventPhase::EXECUTING),
      wants_puzzle_(false), puzzle_level_(0),
      text_char_index_(0), text_timer_(0), current_text_(nullptr),
      left_char_id_(-1), right_char_id_(-1), ui_manager_(text_gen) {
}

void EventState::set_script(const EventScript& script) {
    script_ = &script;
}

void EventState::init(StateManager& /*manager*/) {
    pc_ = 0;
    phase_ = EventPhase::EXECUTING;
    wants_puzzle_ = false;
    puzzle_level_ = 0;
    text_char_index_ = 0;
    text_timer_ = 0;
    current_text_ = nullptr;
    left_char_id_ = -1;
    right_char_id_ = -1;

    ui_manager_.load_screen(ui_data_event::SCREEN);
    ui_.emplace(ui_manager_);
    clear_all();

    // Clear screen fade
    bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
}

void EventState::update(StateManager& manager) {
    switch (phase_) {
        case EventPhase::EXECUTING:
            execute_next();
            break;

        case EventPhase::WAITING_INPUT:
            // Animate text scrolling
            if (current_text_ != nullptr) {
                // Calculate text length
                int text_len = 0;
                {
                    const char* tmp = current_text_;
                    while (tmp[text_len] != 0) { text_len++; }
                }

                if (text_char_index_ < text_len) {
                    // Text speed based on save settings
                    int speed = 1;
                    switch (save_.text_speed) {
                        case 0: speed = 3; break;  // slow
                        case 1: speed = 1; break;  // normal
                        case 2: speed = 0; break;  // fast (instant)
                        default: speed = 1; break;
                    }

                    text_timer_++;
                    if (text_timer_ >= speed) {
                        text_timer_ = 0;
                        text_char_index_++;
                        if (speed == 0) text_char_index_ = text_len; // instant
                        update_dialog_text();
                    }
                }

                // A button: if text still scrolling, show all; if done, advance
                if (bn::keypad::a_pressed()) {
                    if (text_char_index_ < text_len) {
                        text_char_index_ = text_len;
                        update_dialog_text();
                    } else {
                        phase_ = EventPhase::EXECUTING;
                    }
                }
            } else {
                // No text, just waiting for input
                if (bn::keypad::a_pressed()) {
                    phase_ = EventPhase::EXECUTING;
                }
            }
            break;

        case EventPhase::FINISHED:
            manager.pop();
            break;

        default:
            break;
    }

    ui_manager_.update();
}

void EventState::execute_next() {
    if (!script_ || pc_ >= script_->num_commands) {
        phase_ = EventPhase::FINISHED;
        return;
    }

    const EventCommand& cmd = script_->commands[pc_];
    pc_++;

    switch (cmd.cmd) {
        case EventCmd::TEXT:
            current_text_ = cmd.text;
            text_char_index_ = 0;
            text_timer_ = 0;
            update_dialog_text();
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
            save_data_set_flag(save_, cmd.arg1, true);
            break;

        case EventCmd::CHECK_FLAG:
            if (!save_data_get_flag(save_, cmd.arg1)) {
                pc_ += cmd.arg2;  // Skip N commands
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
            sound_.play_move();  // Placeholder
            break;

        default:
            // Unknown commands: skip
            break;
    }
}

void EventState::update_dialog_text() {
    if (!ui_) return;
    if (!current_text_) {
        ui_->set_message("");
        return;
    }

    // Build partial text up to text_char_index_
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

void EventState::shutdown() {
    clear_all();
    ui_manager_.clear_all();
    ui_.reset();
    bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
}
