#ifndef SETTINGS_STATE_H
#define SETTINGS_STATE_H

#include "state/state.h"
#include "save/save_data.h"
#include "audio/sound_manager.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

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
    bn::sprite_text_generator& text_gen_;
    SoundManager& sound_;
    SaveSlot& save_;
    int cursor_;
    bn::vector<bn::sprite_ptr, 32> sprites_;

    void draw_settings();
    static constexpr int SETTINGS_COUNT = static_cast<int>(SettingsItem::COUNT);
};

#endif // SETTINGS_STATE_H
