#include "practice_menu_state.h"
#include "state_manager.h"
#include "bn_keypad.h"

PracticeMenuState::PracticeMenuState(bn::sprite_text_generator& text_gen)
    : text_gen_(text_gen) {
}

void PracticeMenuState::init(StateManager& /*manager*/) {
    sprites_.clear();
    text_gen_.set_center_alignment();
    text_gen_.generate(0, -8, "PRACTICE MODE",  sprites_);
    text_gen_.generate(0,  8, "Coming Soon...", sprites_);
}

void PracticeMenuState::update(StateManager& manager) {
    if (bn::keypad::b_pressed() || bn::keypad::a_pressed()) {
        manager.pop();
    }
}

void PracticeMenuState::shutdown() {
    sprites_.clear();
}
