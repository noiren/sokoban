#include "menu_state.h"
#include "state/Manager/state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "ui_data_mainmenu.h"

MenuState::MenuState()
    : cursor_(0), step_(PhaseStep::OPENING) {
}

void MenuState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_mainmenu::SCREEN);
    step_ = PhaseStep::RUNNING;
}

void MenuState::update(StateManager& sm, SharedContext& ctx) {
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

void MenuState::update_menu(StateManager& sm, SharedContext& ctx) {
    if (bn::keypad::up_pressed()) {
        cursor_--;
        if (cursor_ < 0) cursor_ = VISIBLE_MENU_COUNT - 1;
        if (ctx.sound) ctx.sound->play_move();
    }

    if (bn::keypad::down_pressed()) {
        cursor_++;
        if (cursor_ >= VISIBLE_MENU_COUNT) cursor_ = 0;
        if (ctx.sound) ctx.sound->play_move();
    }

    if (bn::keypad::a_pressed()) {
        last_selected_ = static_cast<MenuItem>(cursor_);
        // ここで各ステートへ遷移
        switch(last_selected_) {
            case MenuItem::STORY: sm.change_state(StateID::EVENT); break;
            case MenuItem::PRACTICE: sm.change_state(StateID::PRACTICE); break;
            case MenuItem::ENDLESS: sm.change_state(StateID::ENDLESS); break;
            case MenuItem::GALLERY: sm.change_state(StateID::GALLERY); break;
            case MenuItem::SETTINGS: sm.change_state(StateID::SETTINGS); break;
            default: break;
        }
    }

    if (bn::keypad::select_pressed()) {
        sm.change_state(StateID::DEBUG_MENU);
    }

    update_menu_ui();
}

void MenuState::update_menu_ui() {
    if (!ui_manager_) return;
    const char* labels[] = { "STORY", "PRACTICE", "ENDLESS", "GALLERY", "SETTINGS" };
    const char* ids[] = { "menu_0", "menu_1", "menu_2", "menu_3", "menu_4" };

    for (int i = 0; i < VISIBLE_MENU_COUNT; i++) {
        bn::string<32> text;
        if (i == cursor_) text.append("> ");
        text.append(labels[i]);
        ui_manager_->set_text(ids[i], text);
    }
}

void MenuState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}
