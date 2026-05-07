#include "settings_state.h"
#include "state/Manager/state_manager.h"
#include "input/input_manager.h"
#include "bn_string.h"
#include "ui_data_settings.h"

const SettingsState::PhaseHandlers SettingsState::phase_table_[] = {
    // MAIN
    { &SettingsState::enter_main, &SettingsState::update_main, &SettingsState::exit_main }
};

SettingsState::SettingsState()
    : cursor_(0), bgm_enabled_(true), se_enabled_(true), text_speed_(1),
      phase_(SettingsPhase::MAIN), step_(PhaseStep::OPENING) {
}

void SettingsState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    ui_.emplace(*ui_manager_);
    
    // 設定値の読み込み (ダミー)
    bgm_enabled_ = true;
    se_enabled_ = true;
    text_speed_ = 1;

    phase_ = SettingsPhase::MAIN;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void SettingsState::update(StateManager& sm, SharedContext& ctx) {
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void SettingsState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    ui_.reset();
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}

void SettingsState::change_phase(SettingsPhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    phase_ = next;

    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void SettingsState::enter_main() {
    ui_manager_->load_screen(ui_data_settings::SCREEN);
    step_ = PhaseStep::RUNNING;
}

void SettingsState::update_main(StateManager& sm, SharedContext& ctx) {
    update_display(ctx);

    auto& inp = InputManager::instance();
    if (inp.is_repeat(Action::MoveUp)) {
        cursor_--;
        if (cursor_ < 0) cursor_ = (int)SettingsItem::COUNT - 1;
    }
    if (inp.is_repeat(Action::MoveDown)) {
        cursor_++;
        if (cursor_ >= (int)SettingsItem::COUNT) cursor_ = 0;
    }

    if (inp.is_triggered(Action::MoveLeft) || inp.is_triggered(Action::MoveRight)) {
        bool is_right = inp.is_triggered(Action::MoveRight);
        SettingsItem item = static_cast<SettingsItem>(cursor_);
        if (item == SettingsItem::BGM) {
            bgm_enabled_ = !bgm_enabled_;
        } else if (item == SettingsItem::SE) {
            se_enabled_ = !se_enabled_;
        } else if (item == SettingsItem::TEXT_SPEED) {
            if (is_right) text_speed_ = (text_speed_ + 1) % 3;
            else text_speed_ = (text_speed_ + 2) % 3;
        }
    }

    if (inp.is_triggered(Action::Cancel)) {
        sm.change_state(StateID::MENU);
    }
}

void SettingsState::exit_main() {}

void SettingsState::update_display(SharedContext& /*ctx*/) {
    if (!ui_.has_value()) return;

    bn::string<32> bgm_str = (cursor_ == (int)SettingsItem::BGM ? "> " : "  ");
    bgm_str.append("BGM: ");
    bgm_str.append(bgm_enabled_ ? "ON" : "OFF");
    ui_.value().set_setting_item(0, bgm_str);

    bn::string<32> se_str = (cursor_ == (int)SettingsItem::SE ? "> " : "  ");
    se_str.append("SE: ");
    se_str.append(se_enabled_ ? "ON" : "OFF");
    ui_.value().set_setting_item(1, se_str);

    bn::string<32> spd_str = (cursor_ == (int)SettingsItem::TEXT_SPEED ? "> " : "  ");
    spd_str.append("TEXT SPD: ");
    spd_str.append(bn::to_string<4>(text_speed_));
    ui_.value().set_setting_item(2, spd_str);
}
