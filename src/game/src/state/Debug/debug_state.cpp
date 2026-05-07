#include "debug_state.h"
#include "state/Manager/state_manager.h"
#include "input/input_manager.h"
#include "bn_string.h"
#include "game/sokoban.h"
#include "game/story_data.h"

DebugState::DebugState()
    : cursor_(0),
      wants_event_(false), event_index_(0),
      wants_puzzle_(false), puzzle_level_(0),
      edit_chapter_(0), edit_event_(0), edit_puzzle_(0),
      step_(PhaseStep::OPENING) {
}

void DebugState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    SaveSlot& save = ctx.save->slots[ctx.active_slot];
    cursor_       = 0;
    wants_event_  = false;
    wants_puzzle_ = false;
    edit_chapter_ = save.story_chapter;
    edit_event_   = 0;
    edit_puzzle_  = 0;
    sprites_.clear();
    draw_menu(ctx);
    step_ = PhaseStep::RUNNING;
}

void DebugState::update(StateManager& sm, SharedContext& ctx) {
    switch (step_) {
        case PhaseStep::OPENING:
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            update_menu(sm, ctx);
            break;

        case PhaseStep::CLOSING:
            break;
    }
}

void DebugState::update_menu(StateManager& sm, SharedContext& ctx) {
    bool changed = false;
    SaveSlot& save = ctx.save->slots[ctx.active_slot];

    auto& inp = InputManager::instance();

    if (inp.is_repeat(Action::MoveUp)) {
        cursor_--;
        if (cursor_ < 0) cursor_ = MENU_COUNT - 1;
        changed = true;
        if (ctx.sound) ctx.sound->play_move();
    }
    if (inp.is_repeat(Action::MoveDown)) {
        cursor_++;
        if (cursor_ >= MENU_COUNT) cursor_ = 0;
        changed = true;
        if (ctx.sound) ctx.sound->play_move();
    }

    // 左右で値を調整
    if (inp.is_triggered(Action::MoveLeft) || inp.is_triggered(Action::MoveRight)) {
        int dir = inp.is_triggered(Action::MoveRight) ? 1 : -1;
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

    // A: 実行
    if (inp.is_triggered(Action::Decide)) {
        DebugItem item = static_cast<DebugItem>(cursor_);
        switch (item) {
            case DebugItem::STORY_CHAPTER:
                save.story_chapter = static_cast<uint8_t>(edit_chapter_);
                if (ctx.sound) ctx.sound->play_clear();
                break;
            case DebugItem::CLEAR_FLAGS:
                for (int i = 0; i < 32; i++) save.flags[i] = 0;
                if (ctx.sound) ctx.sound->play_clear();
                break;
            case DebugItem::RESET_SAVE:
                save_slot_init(save);
                if (ctx.sound) ctx.sound->play_clear();
                break;
            case DebugItem::GOTO_EVENT:
                ctx.story_script_index = edit_event_;
                sm.change_state(StateID::EVENT);
                return;
            case DebugItem::GOTO_PUZZLE:
                // TODO: 実際のパズル遷移
                sm.change_state(StateID::PUZZLE);
                return;
            default:
                break;
        }
        changed = true;
    }

    if (changed) {
        draw_menu(ctx);
    }

    if (inp.is_triggered(Action::Cancel)) {
        sm.change_state(StateID::MENU);
    }
}

void DebugState::draw_menu(SharedContext& ctx) {
    if (!ctx.text_generator) return;
    sprites_.clear();
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -68, "- DEBUG -", sprites_);

    ctx.text_generator->set_left_alignment();
    int y = -44;
    int spacing = 16;

    {
        bn::string<32> line;
        if (cursor_ == 0) line.append(">");
        line.append("CHAPTER:");
        line.append(bn::to_string<4>(edit_chapter_));
        ctx.text_generator->generate(-100, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<32> line;
        if (cursor_ == 1) line.append(">");
        line.append("CLEAR ALL FLAGS");
        ctx.text_generator->generate(-100, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<32> line;
        if (cursor_ == 2) line.append(">");
        line.append("RESET SAVE");
        ctx.text_generator->generate(-100, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<32> line;
        if (cursor_ == 3) line.append(">");
        line.append("EVENT:");
        line.append(bn::to_string<4>(edit_event_));
        ctx.text_generator->generate(-100, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<32> line;
        if (cursor_ == 4) line.append(">");
        line.append("PUZZLE:");
        line.append(bn::to_string<4>(edit_puzzle_));
        ctx.text_generator->generate(-100, y, line, sprites_);
        y += spacing;
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 68, "A:EXEC B:BACK LR:ADJ", sprites_);
}

void DebugState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    sprites_.clear();
}
