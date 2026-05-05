#ifndef EVENT_STATE_H
#define EVENT_STATE_H

#include "state/state.h"
#include "game/event_script.h"
#include "save/save_data.h"
#include "audio/sound_manager.h"
#include "bn_optional.h"
#include "gfx/ui_manager.h"

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
    EventState();

    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void update_event(StateManager& sm, SharedContext& ctx);
    void execute_next(SharedContext& ctx);
    void update_dialog_text(SharedContext& ctx);
    void clear_all();

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

    bn::optional<UIManager> ui_manager_;
    bn::optional<EventUI> ui_;
    PhaseStep step_;
};

#endif // EVENT_STATE_H
