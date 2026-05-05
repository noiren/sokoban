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
    DebugState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void update_menu(StateManager& sm, SharedContext& ctx);
    void draw_menu(SharedContext& ctx);
    static constexpr int MENU_COUNT = static_cast<int>(DebugItem::COUNT);

    int cursor_;
    bn::vector<bn::sprite_ptr, 48> sprites_;

    bool wants_event_;
    int event_index_;
    bool wants_puzzle_;
    int puzzle_level_;

    int edit_chapter_;
    int edit_event_;
    int edit_puzzle_;

    PhaseStep step_;
};

#endif // DEBUG_STATE_H
