#ifndef EVENT_STATE_H
#define EVENT_STATE_H

#include "state/state.h"
#include "game/event_script.h"
#include "save/save_data.h"
#include "audio/sound_manager.h"
#include "bn_optional.h"
#include "../gfx/ui_manager.h"

class EventUI {
public:
    EventUI(UIManager& ui) : ui_(ui) {}

    void set_name(const bn::string_view& name) {
        ui_.set_text("name_box", name);
    }
    
    void set_message(const bn::string_view& msg) {
        ui_.set_text("message_text", msg);
    }
    
    void set_left_char(int image_no) {
        ui_.set_sprite_image("char_left", "chara_portraits", image_no);
        ui_.set_sprite_visible("char_left", true);
    }
    
    void set_right_char(int image_no) {
        ui_.set_sprite_image("char_right", "chara_portraits", image_no);
        ui_.set_sprite_visible("char_right", true);
    }

    void clear_left_char() {
        ui_.set_sprite_visible("char_left", false);
    }

    void clear_right_char() {
        ui_.set_sprite_visible("char_right", false);
    }

    void clear_chars() {
        clear_left_char();
        clear_right_char();
    }
    
    void set_cg(int image_no) {
        (void)image_no;
        // CG would be set dynamically here
        // ui_.set_sprite_image("event_cg", "stills", image_no);
        ui_.set_sprite_visible("event_cg", true);
    }

private:
    UIManager& ui_;
};

// イベント処理の内部フェーズ（EventStateのフェーズ単位ではなくスクリプト実行状態）
enum class EventPhase {
    EXECUTING,       // コマンド実行中
    WAITING_INPUT,   // Aボタン待ち
    FINISHED,        // スクリプト終了
};

class EventState : public State {
public:
    EventState(bn::sprite_text_generator& text_gen, SoundManager& sound, SaveSlot& save);

    void set_script(const EventScript& script);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

    // END コマンド後、GOTO_PUZZLE が発行されたか
    bool wants_puzzle() const { return wants_puzzle_; }
    int puzzle_level() const { return puzzle_level_; }

private:
    void update_event(StateManager& manager);
    void execute_next();
    void update_dialog_text();
    void clear_all();

    bn::sprite_text_generator& text_gen_;
    SoundManager& sound_;
    SaveSlot& save_;

    const EventScript* script_;
    int pc_;
    EventPhase phase_;

    bool wants_puzzle_;
    int puzzle_level_;

    int text_char_index_;
    int text_timer_;
    const char* current_text_;

    int left_char_id_;
    int right_char_id_;

    UIManager ui_manager_;
    bn::optional<EventUI> ui_;
    PhaseStep step_;
};

#endif // EVENT_STATE_H
