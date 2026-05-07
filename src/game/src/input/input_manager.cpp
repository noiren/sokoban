#include "input/input_manager.h"
#include "input/input_misc.h"
#include "bn_keypad.h"

InputManager& InputManager::instance() {
    static InputManager inst;
    return inst;
}

InputManager::InputManager() {
    for (int i = 0; i < ACTION_COUNT; i++) {
        hold_frames_[i] = 0;
    }
}

bool InputManager::check_pressed(Action a) {
    switch (a) {
        case Action::MoveUp:    return bn::keypad::up_pressed();
        case Action::MoveDown:  return bn::keypad::down_pressed();
        case Action::MoveLeft:  return bn::keypad::left_pressed();
        case Action::MoveRight: return bn::keypad::right_pressed();
        case Action::Decide:    return bn::keypad::a_pressed() || bn::keypad::start_pressed();
        case Action::Cancel:    return bn::keypad::b_pressed();
        case Action::OpenMenu:  return bn::keypad::select_pressed();
        default:                return false;
    }
}

bool InputManager::check_held(Action a) {
    switch (a) {
        case Action::MoveUp:    return bn::keypad::up_held();
        case Action::MoveDown:  return bn::keypad::down_held();
        case Action::MoveLeft:  return bn::keypad::left_held();
        case Action::MoveRight: return bn::keypad::right_held();
        case Action::Decide:    return bn::keypad::a_held() || bn::keypad::start_held();
        case Action::Cancel:    return bn::keypad::b_held();
        case Action::OpenMenu:  return bn::keypad::select_held();
        default:                return false;
    }
}

bool InputManager::check_released(Action a) {
    switch (a) {
        case Action::MoveUp:    return bn::keypad::up_released();
        case Action::MoveDown:  return bn::keypad::down_released();
        case Action::MoveLeft:  return bn::keypad::left_released();
        case Action::MoveRight: return bn::keypad::right_released();
        case Action::Decide:    return bn::keypad::a_released() || bn::keypad::start_released();
        case Action::Cancel:    return bn::keypad::b_released();
        case Action::OpenMenu:  return bn::keypad::select_released();
        default:                return false;
    }
}

void InputManager::update() {
    for (int i = 0; i < ACTION_COUNT; i++) {
        if (check_held(static_cast<Action>(i))) {
            hold_frames_[i]++;
        } else {
            hold_frames_[i] = 0;
        }
    }
}

bool InputManager::is_triggered(Action a) const { return check_pressed(a); }
bool InputManager::is_held(Action a)      const { return check_held(a); }
bool InputManager::is_released(Action a)  const { return check_released(a); }

bool InputManager::is_repeat(Action a) const {
    int f = hold_frames_[static_cast<int>(a)];
    if (f == 1) return true;
    if (f > InputMisc::REPEAT_DELAY) {
        return ((f - InputMisc::REPEAT_DELAY) % InputMisc::REPEAT_INTERVAL) == 0;
    }
    return false;
}
