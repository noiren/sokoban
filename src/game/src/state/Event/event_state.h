#ifndef EVENT_STATE_H
#define EVENT_STATE_H

#include "state/state.h"
#include "game/event_script.h"
#include "save/save_data.h"
#include "bn_optional.h"
#include "ui/Core/Manager/ui_manager.h"

class EventUI {
public:
    EventUI(UIManager& ui) : ui_(ui) {}

    void set_name(const bn::string_view& name) {
        if (auto* text_node = ui_.get_text("name_box")) {
            text_node->set_text(name);
        }
    }
    
    void set_message(const bn::string_view& msg) {
        if (auto* text_node = ui_.get_text("message_text")) {
            text_node->set_text(msg);
        }
    }
    
    void set_left_char(int image_no) {
        if (auto* img_node = ui_.get_image("char_left")) {
            ui_.change_sprite_image(img_node, "chara_portraits", image_no);
            img_node->set_visible(true);
        }
    }
    
    void set_right_char(int image_no) {
        if (auto* img_node = ui_.get_image("char_right")) {
            ui_.change_sprite_image(img_node, "chara_portraits", image_no);
            img_node->set_visible(true);
        }
    }

    void clear_left_char() {
        if (auto* img_node = ui_.get_image("char_left")) {
            img_node->set_visible(false);
        }
    }

    void clear_right_char() {
        if (auto* img_node = ui_.get_image("char_right")) {
            img_node->set_visible(false);
        }
    }

    void clear_chars() {
        clear_left_char();
        clear_right_char();
    }
    
    void set_cg(int image_no) {
        (void)image_no;
        if (auto* img_node = ui_.get_image("event_cg")) {
            img_node->set_visible(true);
        }
    }

private:
    UIManager& ui_;
};

enum class EventPhase {
    EXECUTING,
    WAITING_INPUT,
    FINISHED,
    COUNT
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
    void change_phase(EventPhase next);

    void enter_executing();
    void update_executing(StateManager& sm, SharedContext& ctx);
    void exit_executing();

    void enter_waiting();
    void update_waiting(StateManager& sm, SharedContext& ctx);
    void exit_waiting();

    void enter_finished();
    void update_finished(StateManager& sm, SharedContext& ctx);
    void exit_finished();

    void execute_next(SharedContext& ctx);
    void update_dialog_text(SharedContext& ctx);
    void clear_all();

    using EnterExitFunc = void (EventState::*)();
    using UpdateFunc = void (EventState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];

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
