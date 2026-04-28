#ifndef DEBUG_STATE_H
#define DEBUG_STATE_H

#include "state/state.h"
#include "save/save_data.h"
#include "audio/sound_manager.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

enum class DebugItem {
    STORY_CHAPTER,
    CLEAR_FLAGS,
    RESET_SAVE,
    GOTO_EVENT,
    GOTO_PUZZLE,
    COUNT
};

class DebugState : public State {
public:
    DebugState(bn::sprite_text_generator& text_gen, SoundManager& sound, SaveSlot& save);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

    bool wants_event() const { return wants_event_; }
    int event_index() const { return event_index_; }
    bool wants_puzzle() const { return wants_puzzle_; }
    int puzzle_level() const { return puzzle_level_; }

private:
    bn::sprite_text_generator& text_gen_;
    SoundManager& sound_;
    SaveSlot& save_;
    int cursor_;
    bn::vector<bn::sprite_ptr, 48> sprites_;

    bool wants_event_;
    int event_index_;
    bool wants_puzzle_;
    int puzzle_level_;

    // Editable values
    int edit_chapter_;
    int edit_event_;
    int edit_puzzle_;

    void draw_menu();
    static constexpr int MENU_COUNT = static_cast<int>(DebugItem::COUNT);
};

#endif // DEBUG_STATE_H
