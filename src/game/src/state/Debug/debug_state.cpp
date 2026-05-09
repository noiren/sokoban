#include "debug_state.h"

#include "state/Manager/state_manager.h"
#include "input/input_manager.h"
#include "audio/sound_manager.h"
#include "save/save_data.h"
#include "generated/audio_ids.h"
#include "bn_string.h"
#include "game/sokoban.h"
#include "game/story_data.h"

DebugState::DebugState()
    : cursor_(0),
      wants_event_(false),
      event_index_(0),
      wants_puzzle_(false),
      puzzle_level_(0),
      edit_chapter_(0),
      edit_event_(0),
      edit_puzzle_(0),
      edit_audio_bgm_loop_(true),
      edit_audio_bgm_index_(0),
      edit_audio_se_index_(0),
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
    edit_audio_bgm_loop_ = true;
    edit_audio_bgm_index_ = 0;
    edit_audio_se_index_ = 0;
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

namespace {

[[nodiscard]] int bgm_index_max()
{
    int n = static_cast<int>(BN_GENERATED_BGM_COUNT);
    return n > 0 ? n - 1 : 0;
}

[[nodiscard]] int se_index_max()
{
    int n = static_cast<int>(BN_GENERATED_SE_COUNT);
    return n > 0 ? n - 1 : 0;
}

} // namespace

void DebugState::update_menu(StateManager& sm, SharedContext& ctx) {
    bool changed = false;
    SaveSlot& save = ctx.save->slots[ctx.active_slot];

    auto& inp = InputManager::instance();

    if (inp.is_repeat(Action::MoveUp)) {
        cursor_--;
        if (cursor_ < 0) cursor_ = MENU_COUNT - 1;
        changed = true;
        SoundManager::instance().play_move();
    }
    if (inp.is_repeat(Action::MoveDown)) {
        cursor_++;
        if (cursor_ >= MENU_COUNT) cursor_ = 0;
        changed = true;
        SoundManager::instance().play_move();
    }

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
            case DebugItem::AUDIO_BGM_LOOP:
                edit_audio_bgm_loop_ = !edit_audio_bgm_loop_;
                break;
            case DebugItem::AUDIO_BGM_INDEX:
                if (BN_GENERATED_BGM_COUNT > 0) {
                    edit_audio_bgm_index_ += dir;
                    if (edit_audio_bgm_index_ < 0) edit_audio_bgm_index_ = bgm_index_max();
                    if (edit_audio_bgm_index_ > bgm_index_max()) edit_audio_bgm_index_ = 0;
                }
                break;
            case DebugItem::AUDIO_SE_INDEX:
                if (BN_GENERATED_SE_COUNT > 0) {
                    edit_audio_se_index_ += dir;
                    if (edit_audio_se_index_ < 0) edit_audio_se_index_ = se_index_max();
                    if (edit_audio_se_index_ > se_index_max()) edit_audio_se_index_ = 0;
                }
                break;
            default:
                break;
        }
        changed = true;
    }

    if (inp.is_triggered(Action::Decide)) {
        DebugItem item = static_cast<DebugItem>(cursor_);
        switch (item) {
            case DebugItem::STORY_CHAPTER:
                save.story_chapter = static_cast<uint8_t>(edit_chapter_);
                SoundManager::instance().play_clear();
                break;
            case DebugItem::CLEAR_FLAGS:
                for (int i = 0; i < 32; i++) save.flags[i] = 0;
                SoundManager::instance().play_clear();
                break;
            case DebugItem::RESET_SAVE:
                save_slot_init(save);
                SoundManager::instance().set_bgm_enabled(save.bgm_enabled);
                SoundManager::instance().set_se_enabled(save.se_enabled);
                SoundManager::instance().play_clear();
                break;
            case DebugItem::GOTO_EVENT:
                ctx.story_script_index = edit_event_;
                sm.change_state(StateID::EVENT);
                return;
            case DebugItem::GOTO_PUZZLE:
                sm.change_state(StateID::PUZZLE);
                return;
            case DebugItem::AUDIO_PLAY_BGM:
                if (BN_GENERATED_BGM_COUNT > 0) {
                    SoundManager::instance().play_bgm(
                        static_cast<BgmId>(edit_audio_bgm_index_),
                        edit_audio_bgm_loop_,
                        45,
                        true);
                }
                break;
            case DebugItem::AUDIO_STOP_BGM:
                SoundManager::instance().stop_bgm(45);
                break;
            case DebugItem::AUDIO_PLAY_SE:
                if (BN_GENERATED_SE_COUNT > 0) {
                    SoundManager::instance().play_se(static_cast<SeId>(edit_audio_se_index_));
                }
                break;
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
    const int spacing = 14;

    {
        bn::string<40> line;
        if (cursor_ == 0) line.append(">");
        line.append("CHAPTER:");
        line.append(bn::to_string<4>(edit_chapter_));
        ctx.text_generator->generate(-108, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<40> line;
        if (cursor_ == 1) line.append(">");
        line.append("CLEAR ALL FLAGS");
        ctx.text_generator->generate(-108, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<40> line;
        if (cursor_ == 2) line.append(">");
        line.append("RESET SAVE");
        ctx.text_generator->generate(-108, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<40> line;
        if (cursor_ == 3) line.append(">");
        line.append("EVENT:");
        line.append(bn::to_string<4>(edit_event_));
        ctx.text_generator->generate(-108, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<40> line;
        if (cursor_ == 4) line.append(">");
        line.append("PUZZLE:");
        line.append(bn::to_string<4>(edit_puzzle_));
        ctx.text_generator->generate(-108, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<40> line;
        if (cursor_ == 5) line.append(">");
        line.append("BGM LOOP:");
        line.append(edit_audio_bgm_loop_ ? "Y" : "N");
        ctx.text_generator->generate(-108, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<40> line;
        if (cursor_ == 6) line.append(">");
        line.append("BGM IDX:");
        if (BN_GENERATED_BGM_COUNT == 0) {
            line.append("(none)");
        } else {
            line.append(bn::to_string<4>(edit_audio_bgm_index_));
            line.append("/");
            line.append(bn::to_string<4>(bgm_index_max()));
        }
        ctx.text_generator->generate(-108, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<40> line;
        if (cursor_ == 7) line.append(">");
        line.append("PLAY BGM (fade in)");
        ctx.text_generator->generate(-108, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<40> line;
        if (cursor_ == 8) line.append(">");
        line.append("STOP BGM (fade out)");
        ctx.text_generator->generate(-108, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<40> line;
        if (cursor_ == 9) line.append(">");
        line.append("SE IDX:");
        if (BN_GENERATED_SE_COUNT == 0) {
            line.append("(none)");
        } else {
            line.append(bn::to_string<4>(edit_audio_se_index_));
            line.append("/");
            line.append(bn::to_string<4>(se_index_max()));
        }
        ctx.text_generator->generate(-108, y, line, sprites_);
        y += spacing;
    }
    {
        bn::string<40> line;
        if (cursor_ == 10) line.append(">");
        line.append("PLAY SE");
        ctx.text_generator->generate(-108, y, line, sprites_);
        y += spacing;
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 76, "A:DO B:BACK LR:ADJ", sprites_);
}

void DebugState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    sprites_.clear();
}
