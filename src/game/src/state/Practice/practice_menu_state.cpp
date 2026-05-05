#include "practice_menu_state.h"
#include "state/Manager/state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "ui_data_practice_stageSelect.h"
#include "game/sokoban.h"

PracticeMenuState::PracticeMenuState()
    : cursor_(0), step_(PhaseStep::OPENING) {
}

void PracticeMenuState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_practice_stageSelect::SCREEN);
    step_ = PhaseStep::RUNNING;
}

void PracticeMenuState::update(StateManager& sm, SharedContext& ctx) {
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

void PracticeMenuState::update_menu(StateManager& sm, SharedContext& /*ctx*/) {
    if (bn::keypad::up_pressed()) {
        cursor_--;
        if (cursor_ < 0) cursor_ = 0;
    }

    if (bn::keypad::down_pressed()) {
        int max_levels = get_num_levels();
        cursor_++;
        if (cursor_ >= max_levels) cursor_ = max_levels - 1;
    }

    if (bn::keypad::a_pressed()) {
        selected_level_ = cursor_;
        sm.change_state(StateID::MENU); // TODO: 本来はパズル画面へ
    }

    if (bn::keypad::b_pressed()) {
        sm.change_state(StateID::MENU);
    }
}

void PracticeMenuState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}
