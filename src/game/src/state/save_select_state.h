#ifndef SAVE_SELECT_STATE_H
#define SAVE_SELECT_STATE_H

#include "state/state.h"
#include "save/save_data.h"
#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "gfx/ui_manager.h"
#include "gfx/fade_effect.h"

class SaveSelectUI {
public:
    SaveSelectUI(UIManager& ui) : ui_(ui) {}
    void set_slot_text(int index, const bn::string_view& text) {
        if (index == 0) ui_.set_text("slot_0", text);
        else if (index == 1) ui_.set_text("slot_1", text);
        else if (index == 2) ui_.set_text("slot_2", text);
    }
private:
    UIManager& ui_;
};

enum class SaveSelectPhase {
    SELECT_SLOT, // スロット選択
    COUNT
};

class SaveSelectState : public State {
public:
    SaveSelectState();

    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void change_phase(SaveSelectPhase next);

    void enter_select();
    void update_select(StateManager& sm, SharedContext& ctx);
    void exit_select();

    void update_slots_ui(SharedContext& ctx);

    using EnterExitFunc = void (SaveSelectState::*)();
    using UpdateFunc = void (SaveSelectState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];

    int cursor_;
    int selected_slot_;
    FadeEffect fade_;
    bn::optional<UIManager> ui_manager_;
    bn::optional<SaveSelectUI> ui_;
    SaveSelectPhase phase_;
    PhaseStep step_;

    static constexpr int FADE_FRAMES = 30;
};

#endif // SAVE_SELECT_STATE_H
