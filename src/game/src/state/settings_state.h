#ifndef SETTINGS_STATE_H
#define SETTINGS_STATE_H

#include "state/state.h"
#include "save/save_data.h"
#include "audio/sound_manager.h"
#include "bn_optional.h"
#include "../gfx/ui_manager.h"

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
    SettingsState(bn::sprite_text_generator& text_gen, SoundManager& sound, SaveSlot& save);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

private:
    void update_edit(StateManager& manager);
    void update_settings_ui();

    bn::sprite_text_generator& text_gen_;
    SoundManager& sound_;
    SaveSlot& save_;
    int cursor_;
    UIManager ui_manager_;
    bn::optional<SettingsUI> ui_;
    PhaseStep step_;
    static constexpr int SETTINGS_COUNT = static_cast<int>(SettingsItem::COUNT);
};

#endif // SETTINGS_STATE_H
