#include "practice_menu_state.h"
#include "state/Manager/state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "ui_data_practice_stageSelect.h"
#include "game/sokoban.h"

const PracticeMenuState::PhaseHandlers PracticeMenuState::phase_table_[] = {
    // SELECT_LEVEL
    { &PracticeMenuState::enter_select, &PracticeMenuState::update_select, &PracticeMenuState::exit_select }
};

PracticeMenuState::PracticeMenuState()
    : cursor_(0), phase_(PracticeMenuPhase::SELECT_LEVEL), step_(PhaseStep::OPENING) {
}

void PracticeMenuState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    
    phase_ = PracticeMenuPhase::SELECT_LEVEL;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void PracticeMenuState::update(StateManager& sm, SharedContext& ctx) {
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void PracticeMenuState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}

void PracticeMenuState::change_phase(PracticeMenuPhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    phase_ = next;

    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void PracticeMenuState::enter_select() {
    ui_manager_->load_screen(ui_data_practice_stageSelect::SCREEN);
    step_ = PhaseStep::RUNNING;
}

void PracticeMenuState::update_select(StateManager& sm, SharedContext& /*ctx*/) {
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

void PracticeMenuState::exit_select() {}
