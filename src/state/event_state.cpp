#include "event_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "bn_bg_palettes.h"

EventState::EventState(bn::sprite_text_generator& text_gen, SoundManager& sound, SaveSlot& save)
    : text_gen_(text_gen), sound_(sound), save_(save),
      script_(nullptr), pc_(0), phase_(EventPhase::EXECUTING),
      wants_puzzle_(false), puzzle_level_(0),
      text_char_index_(0), text_timer_(0), current_text_(nullptr),
      left_char_id_(-1), right_char_id_(-1) {
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
    dialog_sprites_.clear();
    label_sprites_.clear();

    // Clear screen
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
                        draw_dialog();
                    }
                }

                // A button: if text still scrolling, show all; if done, advance
                if (bn::keypad::a_pressed()) {
                    if (text_char_index_ < text_len) {
                        text_char_index_ = text_len;
                        draw_dialog();
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
            draw_dialog();
            draw_characters();
            phase_ = EventPhase::WAITING_INPUT;
            break;

        case EventCmd::WAIT_INPUT:
            current_text_ = nullptr;
            phase_ = EventPhase::WAITING_INPUT;
            break;

        case EventCmd::CLEAR_TEXT:
            dialog_sprites_.clear();
            current_text_ = nullptr;
            break;

        case EventCmd::SHOW_LEFT:
            left_char_id_ = cmd.arg1;
            draw_characters();
            break;

        case EventCmd::SHOW_RIGHT:
            right_char_id_ = cmd.arg1;
            draw_characters();
            break;

        case EventCmd::HIDE_LEFT:
            left_char_id_ = -1;
            draw_characters();
            break;

        case EventCmd::HIDE_RIGHT:
            right_char_id_ = -1;
            draw_characters();
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

void EventState::draw_dialog() {
    dialog_sprites_.clear();

    if (!current_text_) return;

    // Build partial text up to text_char_index_
    bn::string<64> partial;
    int count = 0;
    const char* p = current_text_;
    while (*p && count < text_char_index_) {
        partial.append(*p);
        p++;
        count++;
    }

    // Draw dialog text at bottom of screen
    // GBA screen: 240x160, center=(0,0), bottom area is around y=48~72
    text_gen_.set_left_alignment();
    text_gen_.generate(-112, 48, partial, dialog_sprites_);
}

void EventState::draw_characters() {
    label_sprites_.clear();
    text_gen_.set_center_alignment();

    // Placeholder: show character ID as text sprites
    // In real version, these would be sprite images
    if (left_char_id_ >= 0) {
        bn::string<16> name = "CHAR:";
        name.append(bn::to_string<4>(left_char_id_));
        text_gen_.generate(-60, -16, name, label_sprites_);
    }

    if (right_char_id_ >= 0) {
        bn::string<16> name = "CHAR:";
        name.append(bn::to_string<4>(right_char_id_));
        text_gen_.generate(60, -16, name, label_sprites_);
    }
}

void EventState::clear_all() {
    dialog_sprites_.clear();
    label_sprites_.clear();
}

void EventState::shutdown() {
    clear_all();
    bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
}
