#ifndef SETTINGS_STATE_H
#define SETTINGS_STATE_H

#include "state/state.h"
#include "bn_optional.h"
#include "ui/Core/Manager/ui_manager.h"

class SettingsUI {
public:
    SettingsUI(UIManager& ui) : ui_(ui) {}
    void set_setting_item(int index, const bn::string_view& text) {
        if (index == 0) {
            if (auto* text_node = ui_.get_text("setting_0")) text_node->set_text(text);
        } else if (index == 1) {
            if (auto* text_node = ui_.get_text("setting_1")) text_node->set_text(text);
        } else if (index == 2) {
            if (auto* text_node = ui_.get_text("setting_2")) text_node->set_text(text);
        }
    }
private:
    UIManager& ui_;
};

enum class SettingsItem {
    BGM,
    SE,
    TEXT_SPEED,
    COUNT
};

enum class SettingsPhase {
    MAIN,
    COUNT
};

class SettingsState : public State {
public:
    SettingsState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void change_phase(SettingsPhase next);

    void enter_main();
    void update_main(StateManager& sm, SharedContext& ctx);
    void exit_main();

    void update_display(SharedContext& ctx);

    using EnterExitFunc = void (SettingsState::*)();
    using UpdateFunc = void (SettingsState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];

    int cursor_;
    bool bgm_enabled_;
    bool se_enabled_;
    int text_speed_;
    bn::optional<UIManager> ui_manager_;
    bn::optional<SettingsUI> ui_;
    SettingsPhase phase_;
    PhaseStep step_;
};

#endif // SETTINGS_STATE_H
