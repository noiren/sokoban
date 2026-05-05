#include "settings_state.h"
#include "state/Manager/state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "ui_data_settings.h"

SettingsState::SettingsState()
    : cursor_(0), step_(PhaseStep::OPENING) {
}

void SettingsState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    SaveSlot& save = ctx.save->slots[ctx.active_slot];
    bgm_enabled_ = save.bgm_enabled;
    se_enabled_  = save.se_enabled;
    text_speed_  = save.text_speed;
    cursor_      = 0;

    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_settings::SCREEN);
    if (ctx.sound) ctx.sound->set_se_enabled(se_enabled_);
    update_display(ctx);
    step_ = PhaseStep::RUNNING;
}

void SettingsState::update(StateManager& sm, SharedContext& ctx) {
    switch (step_) {
        case PhaseStep::OPENING:
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            update_menu(sm, ctx);
            break;

        case PhaseStep::CLOSING:
            break;
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void SettingsState::update_menu(StateManager& sm, SharedContext& ctx) {
    constexpr int NUM_ITEMS = 3;

    if (bn::keypad::up_pressed()) {
        cursor_--;
        if (cursor_ < 0) cursor_ = NUM_ITEMS - 1;
        if (ctx.sound) ctx.sound->play_move();
    }

    if (bn::keypad::down_pressed()) {
        cursor_++;
        if (cursor_ >= NUM_ITEMS) cursor_ = 0;
        if (ctx.sound) ctx.sound->play_move();
    }

    // 左右で値変更
    if (bn::keypad::left_pressed() || bn::keypad::right_pressed()) {
        int dir = bn::keypad::right_pressed() ? 1 : -1;
        switch (cursor_) {
            case 0:  // BGM
                bgm_enabled_ = !bgm_enabled_;
                break;
            case 1:  // SE
                se_enabled_ = !se_enabled_;
                if (ctx.sound) {
                    ctx.sound->set_se_enabled(se_enabled_);
                    ctx.sound->play_move();
                }
                break;
            case 2:  // テキスト速度
                text_speed_ += dir;
                if (text_speed_ < 0) text_speed_ = 0;
                if (text_speed_ > 2) text_speed_ = 2;
                break;
        }
        update_display(ctx);
    }

    // A: 変更を保存して戻る
    if (bn::keypad::a_pressed()) {
        SaveSlot& save = ctx.save->slots[ctx.active_slot];
        save.bgm_enabled = bgm_enabled_;
        save.se_enabled  = se_enabled_;
        save.text_speed  = static_cast<uint8_t>(text_speed_);
        sm.change_state(StateID::MENU);
    }

    // B: 戻る
    if (bn::keypad::b_pressed()) {
        sm.change_state(StateID::MENU);
    }
}

void SettingsState::update_display(SharedContext& /*ctx*/) {
    if (!ui_manager_) return;
    bn::string<32> bgm_str = "BGM: ";
    bgm_str.append(bgm_enabled_ ? "ON" : "OFF");
    ui_manager_->set_text("bgm_toggle", bgm_str);

    bn::string<32> se_str = "SE: ";
    se_str.append(se_enabled_ ? "ON" : "OFF");
    ui_manager_->set_text("se_toggle", se_str);

    bn::string<32> speed_str = "TEXT: ";
    switch (text_speed_) {
        case 0:  speed_str.append("SLOW");  break;
        case 1:  speed_str.append("NORMAL"); break;
        case 2:  speed_str.append("FAST");   break;
        default: speed_str.append("NORMAL"); break;
    }
    ui_manager_->set_text("text_speed", speed_str);
}

void SettingsState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}
