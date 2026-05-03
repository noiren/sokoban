#include "practice_menu_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "ui_data_practice_stageSelect.h"

PracticeMenuState::PracticeMenuState(bn::sprite_text_generator& text_gen)
    : ui_manager_(text_gen), step_(PhaseStep::OPENING) {
}

void PracticeMenuState::init(StateManager& /*manager*/) {
    ui_manager_.load_screen(ui_data_practice_stageSelect::SCREEN);
    step_ = PhaseStep::RUNNING;  // 現時点はフェードなしで即開始
}

void PracticeMenuState::update(StateManager& manager) {
    switch (step_) {
        case PhaseStep::OPENING:
            // TODO: フェードイン処理
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            update_select(manager);
            break;

        case PhaseStep::CLOSING:
            // TODO: フェードアウト処理
            manager.pop();
            break;
    }

    ui_manager_.update();
}

void PracticeMenuState::update_select(StateManager& /*manager*/) {
    if (bn::keypad::b_pressed() || bn::keypad::a_pressed()) {
        step_ = PhaseStep::CLOSING;
    }
}

void PracticeMenuState::shutdown() {
    ui_manager_.clear_all();
}
