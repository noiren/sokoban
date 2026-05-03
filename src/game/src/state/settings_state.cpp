#include "settings_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "ui_data_settings.h"

SettingsState::SettingsState(bn::sprite_text_generator& text_gen, SoundManager& sound, SaveSlot& save)
    : text_gen_(text_gen), sound_(sound), save_(save),
      cursor_(0), ui_manager_(text_gen), step_(PhaseStep::OPENING) {
}

void SettingsState::init(StateManager& /*manager*/) {
    cursor_ = 0;

    ui_manager_.load_screen(ui_data_settings::SCREEN);
    ui_.emplace(ui_manager_);

    update_settings_ui();
    step_ = PhaseStep::RUNNING;  // 現時点はフェードなしで即開始
}

void SettingsState::update(StateManager& manager) {
    switch (step_) {
        case PhaseStep::OPENING:
            // TODO: フェードイン処理
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            update_edit(manager);
            break;

        case PhaseStep::CLOSING:
            // TODO: フェードアウト処理（セーブはmain.cpp側で行う）
            manager.pop();
            break;
    }

    ui_manager_.update();
}

void SettingsState::update_edit(StateManager& /*manager*/) {
    bool changed = false;

    if (bn::keypad::up_pressed()) {
        cursor_--;
        if (cursor_ < 0) cursor_ = SETTINGS_COUNT - 1;
        changed = true;
        sound_.play_move();
    }
    if (bn::keypad::down_pressed()) {
        cursor_++;
        if (cursor_ >= SETTINGS_COUNT) cursor_ = 0;
        changed = true;
        sound_.play_move();
    }

    if (bn::keypad::a_pressed() || bn::keypad::left_pressed() || bn::keypad::right_pressed()) {
        SettingsItem item = static_cast<SettingsItem>(cursor_);
        switch (item) {
            case SettingsItem::BGM:
                save_.bgm_enabled = !save_.bgm_enabled;
                break;
            case SettingsItem::SE:
                save_.se_enabled = !save_.se_enabled;
                sound_.set_se_enabled(save_.se_enabled);
                break;
            case SettingsItem::TEXT_SPEED:
                save_.text_speed = (save_.text_speed + 1) % 3;
                break;
            default:
                break;
        }
        changed = true;
    }

    if (changed) {
        update_settings_ui();
    }

    if (bn::keypad::b_pressed()) {
        step_ = PhaseStep::CLOSING;
    }
}

void SettingsState::update_settings_ui() {
    if (!ui_) return;

    const char* speed_labels[] = { "SLOW", "NORMAL", "FAST" };

    // BGM
    {
        bn::string<32> line;
        if (cursor_ == 0) line.append("> ");
        line.append("BGM: ");
        line.append(save_.bgm_enabled ? "ON" : "OFF");
        if (cursor_ == 0) line.append(" <");
        ui_->set_setting_item(0, line);
    }

    // SE
    {
        bn::string<32> line;
        if (cursor_ == 1) line.append("> ");
        line.append("SE:  ");
        line.append(save_.se_enabled ? "ON" : "OFF");
        if (cursor_ == 1) line.append(" <");
        ui_->set_setting_item(1, line);
    }

    // Text Speed
    {
        bn::string<32> line;
        if (cursor_ == 2) line.append("> ");
        line.append("TEXT:");
        line.append(speed_labels[save_.text_speed]);
        if (cursor_ == 2) line.append(" <");
        ui_->set_setting_item(2, line);
    }
}

void SettingsState::shutdown() {
    ui_manager_.clear_all();
    ui_.reset();
}
