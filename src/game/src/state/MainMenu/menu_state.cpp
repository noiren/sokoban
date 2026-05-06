#include "menu_state.h"
#include "state/Manager/state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "ui_data_menu.h"

const MenuState::PhaseHandlers MenuState::phase_table_[] = {
    // MAIN
    { &MenuState::enter_main, &MenuState::update_main, &MenuState::exit_main }
};

MenuState::MenuState()
    : cursor_(0), last_selected_(MenuItem::STORY),
      phase_(MenuPhase::MAIN), step_(PhaseStep::OPENING) {
}

void MenuState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    ui_.emplace(*ui_manager_);

    phase_ = MenuPhase::MAIN;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void MenuState::update(StateManager& sm, SharedContext& ctx) {
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void MenuState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    ui_.reset();
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}

void MenuState::change_phase(MenuPhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    phase_ = next;

    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void MenuState::enter_main() {
    ui_manager_->load_screen(ui_data_menu::SCREEN);
    cursor_ = (int)last_selected_;
    if (cursor_ >= VISIBLE_MENU_COUNT) cursor_ = 0;
    update_menu_ui();
    step_ = PhaseStep::RUNNING;
}

void MenuState::update_main(StateManager& sm, SharedContext& /*ctx*/) {
    if (bn::keypad::up_pressed()) {
        cursor_--;
        if (cursor_ < 0) cursor_ = VISIBLE_MENU_COUNT - 1;
        update_menu_ui();
    }

    if (bn::keypad::down_pressed()) {
        cursor_++;
        if (cursor_ >= VISIBLE_MENU_COUNT) cursor_ = 0;
        update_menu_ui();
    }

    // DEBUG用隠しコマンド（SELECT）
    if (bn::keypad::select_pressed()) {
        last_selected_ = MenuItem::DEBUG;
        sm.change_state(StateID::DEBUG_MENU);
        return;
    }

    if (bn::keypad::a_pressed()) {
        MenuItem selected = static_cast<MenuItem>(cursor_);
        last_selected_ = selected;
        switch (selected) {
            case MenuItem::STORY:
                sm.change_state(StateID::EVENT); // 最初はイベントから
                break;
            case MenuItem::PRACTICE:
                sm.change_state(StateID::PRACTICE);
                break;
            case MenuItem::ENDLESS:
                sm.change_state(StateID::ENDLESS);
                break;
            case MenuItem::GALLERY:
                sm.change_state(StateID::GALLERY);
                break;
            case MenuItem::SETTINGS:
                sm.change_state(StateID::SETTINGS);
                break;
            default:
                break;
        }
    }

    if (bn::keypad::b_pressed()) {
        sm.change_state(StateID::TITLE);
    }
}

void MenuState::exit_main() {}

void MenuState::update_menu_ui() {
    if (!ui_.has_value()) return;

    for (int i = 0; i < VISIBLE_MENU_COUNT; i++) {
        bn::string<32> text;
        if (i == cursor_) text.append("> ");

        switch (static_cast<MenuItem>(i)) {
            case MenuItem::STORY:    text.append("STORY MODE"); break;
            case MenuItem::PRACTICE: text.append("PRACTICE"); break;
            case MenuItem::ENDLESS:  text.append("ENDLESS"); break;
            case MenuItem::GALLERY:  text.append("GALLERY"); break;
            case MenuItem::SETTINGS: text.append("SETTINGS"); break;
            default: break;
        }
        ui_.value().set_menu_item(i, text);
    }
}
