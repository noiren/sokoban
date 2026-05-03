#include "practice_menu_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "ui_data_practice_stageSelect.h"

PracticeMenuState::PracticeMenuState(bn::sprite_text_generator& text_gen)
    : ui_manager_(text_gen) {
}

void PracticeMenuState::init(StateManager& /*manager*/) {
    ui_manager_.load_screen(ui_data_practice_stageSelect::SCREEN);
}

void PracticeMenuState::update(StateManager& manager) {
    if (bn::keypad::b_pressed() || bn::keypad::a_pressed()) {
        manager.pop();
    }
    ui_manager_.update();
}

void PracticeMenuState::shutdown() {
    ui_manager_.clear_all();
}
