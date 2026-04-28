#include "debug_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "game/sokoban.h"
#include "game/story_data.h"

DebugState::DebugState(bn::sprite_text_generator& text_gen, SoundManager& sound, SaveSlot& save)
    : text_gen_(text_gen), sound_(sound), save_(save), cursor_(0),
      wants_event_(false), event_index_(0),
      wants_puzzle_(false), puzzle_level_(0),
      edit_chapter_(0), edit_event_(0), edit_puzzle_(0) {
}

void DebugState::init(StateManager& /*manager*/) {
    cursor_ = 0;
    wants_event_ = false;
    wants_puzzle_ = false;
    edit_chapter_ = save_.story_chapter;
    edit_event_ = 0;
    edit_puzzle_ = 0;
    sprites_.clear();
    draw_menu();
}

void DebugState::update(StateManager& manager) {
    bool changed = false;

    if (bn::keypad::up_pressed()) {
        cursor_--;
        if (cursor_ < 0) cursor_ = MENU_COUNT - 1;
        changed = true;
        sound_.play_move();
    }
    if (bn::keypad::down_pressed()) {
        cursor_++;
        if (cursor_ >= MENU_COUNT) cursor_ = 0;
        changed = true;
        sound_.play_move();
    }

    // Left/Right to adjust values
    if (bn::keypad::left_pressed() || bn::keypad::right_pressed()) {
        int dir = bn::keypad::right_pressed() ? 1 : -1;
        DebugItem item = static_cast<DebugItem>(cursor_);
        switch (item) {
            case DebugItem::STORY_CHAPTER:
                edit_chapter_ += dir;
                if (edit_chapter_ < 0) edit_chapter_ = 0;
                if (edit_chapter_ > 255) edit_chapter_ = 255;
                break;
            case DebugItem::GOTO_EVENT:
                edit_event_ += dir;
                if (edit_event_ < 0) edit_event_ = 0;
                if (edit_event_ >= NUM_STORY_SCRIPTS) edit_event_ = NUM_STORY_SCRIPTS - 1;
                break;
            case DebugItem::GOTO_PUZZLE:
                edit_puzzle_ += dir;
                if (edit_puzzle_ < 0) edit_puzzle_ = 0;
                if (edit_puzzle_ >= get_num_levels()) edit_puzzle_ = get_num_levels() - 1;
                break;
            default:
                break;
        }
        changed = true;
    }

    // A button = execute
    if (bn::keypad::a_pressed()) {
        DebugItem item = static_cast<DebugItem>(cursor_);
        switch (item) {
            case DebugItem::STORY_CHAPTER:
                save_.story_chapter = static_cast<uint8_t>(edit_chapter_);
                // セーブはmain.cpp側で行う
                sound_.play_clear();
                break;
            case DebugItem::CLEAR_FLAGS:
                for (int i = 0; i < 32; i++) save_.flags[i] = 0;
                // セーブはmain.cpp側で行う
                sound_.play_clear();
                break;
            case DebugItem::RESET_SAVE:
                save_slot_init(save_);
                sound_.play_clear();
                break;
            case DebugItem::GOTO_EVENT:
                wants_event_ = true;
                event_index_ = edit_event_;
                manager.pop();
                return;
            case DebugItem::GOTO_PUZZLE:
                wants_puzzle_ = true;
                puzzle_level_ = edit_puzzle_;
                manager.pop();
                return;
            default:
                break;
        }
        changed = true;
    }

    if (changed) {
        draw_menu();
    }

    if (bn::keypad::b_pressed()) {
        manager.pop();
    }
}

void DebugState::draw_menu() {
    sprites_.clear();
    text_gen_.set_center_alignment();
    text_gen_.generate(0, -68, "- DEBUG -", sprites_);

    text_gen_.set_left_alignment();
    int y = -44;
    int spacing = 16;

    // Story Chapter
    {
        bn::string<32> line;
        if (cursor_ == 0) line.append(">");
        line.append("CHAPTER:");
        line.append(bn::to_string<4>(edit_chapter_));
        text_gen_.generate(-100, y, line, sprites_);
        y += spacing;
    }

    // Clear Flags
    {
        bn::string<32> line;
        if (cursor_ == 1) line.append(">");
        line.append("CLEAR ALL FLAGS");
        text_gen_.generate(-100, y, line, sprites_);
        y += spacing;
    }

    // Reset Save
    {
        bn::string<32> line;
        if (cursor_ == 2) line.append(">");
        line.append("RESET SAVE");
        text_gen_.generate(-100, y, line, sprites_);
        y += spacing;
    }

    // Goto Event
    {
        bn::string<32> line;
        if (cursor_ == 3) line.append(">");
        line.append("EVENT:");
        line.append(bn::to_string<4>(edit_event_));
        text_gen_.generate(-100, y, line, sprites_);
        y += spacing;
    }

    // Goto Puzzle
    {
        bn::string<32> line;
        if (cursor_ == 4) line.append(">");
        line.append("PUZZLE:");
        line.append(bn::to_string<4>(edit_puzzle_));
        text_gen_.generate(-100, y, line, sprites_);
        y += spacing;
    }

    text_gen_.set_center_alignment();
    text_gen_.generate(0, 68, "A:EXEC B:BACK LR:ADJ", sprites_);
}

void DebugState::shutdown() {
    sprites_.clear();
}
