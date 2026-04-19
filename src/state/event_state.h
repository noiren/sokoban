#ifndef EVENT_STATE_H
#define EVENT_STATE_H

#include "state/state.h"
#include "game/event_script.h"
#include "save/save_data.h"
#include "audio/sound_manager.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

// Sub-phases of event processing
enum class EventPhase {
    EXECUTING,       // Processing commands
    WAITING_INPUT,   // Waiting for A button
    FINISHED,        // Script ended
};

class EventState : public State {
public:
    EventState(bn::sprite_text_generator& text_gen, SoundManager& sound, SaveSlot& save);

    void set_script(const EventScript& script);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

    // After END command, did we get a GOTO_PUZZLE?
    bool wants_puzzle() const { return wants_puzzle_; }
    int puzzle_level() const { return puzzle_level_; }

private:
    bn::sprite_text_generator& text_gen_;
    SoundManager& sound_;
    SaveSlot& save_;

    const EventScript* script_;
    int pc_;                     // Program counter
    EventPhase phase_;

    bool wants_puzzle_;
    int puzzle_level_;

    // Text display state
    int text_char_index_;        // Current character being displayed
    int text_timer_;             // Timer for text scroll speed
    const char* current_text_;

    // Visual elements
    bn::vector<bn::sprite_ptr, 64> dialog_sprites_;
    bn::vector<bn::sprite_ptr, 16> label_sprites_;

    // Character display names (for placeholder text rendering)
    int left_char_id_;
    int right_char_id_;

    void execute_next();
    void draw_dialog();
    void draw_characters();
    void clear_all();
};

#endif // EVENT_STATE_H
