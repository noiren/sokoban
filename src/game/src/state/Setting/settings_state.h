#ifndef SETTINGS_STATE_H
#define SETTINGS_STATE_H

#include "state/state.h"
#include "save/save_data.h"
#include "audio/sound_manager.h"
#include "bn_optional.h"
#include "gfx/ui_manager.h"

class SettingsUI {
public:
    SettingsUI(UIManager& ui) : ui_(ui) {}
    void set_setting_item(int index, const bn::string_view& text) {
        if (index == 0) ui_.set_text("setting_0", text);
        else if (index == 1) ui_.set_text("setting_1", text);
        else if (index == 2) ui_.set_text("setting_2", text);
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

class SettingsState : public State {
public:
    SettingsState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void update_menu(StateManager& sm, SharedContext& ctx);
    void update_display(SharedContext& ctx);

    int cursor_;
    bool bgm_enabled_;
    bool se_enabled_;
    int text_speed_;
    bn::optional<UIManager> ui_manager_;
    bn::optional<SettingsUI> ui_;
    PhaseStep step_;
};

#endif // SETTINGS_STATE_H
