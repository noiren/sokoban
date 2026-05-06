#include "puzzle_state.h"
#include "state/Manager/state_manager.h"
#include "bn_keypad.h"

const PuzzleState::PhaseHandlers PuzzleState::phase_table_[] = {
    // PLAYING
    { &PuzzleState::enter_playing, &PuzzleState::update_playing, &PuzzleState::exit_playing },
    // CLEARED
    { &PuzzleState::enter_cleared, &PuzzleState::update_cleared, &PuzzleState::exit_cleared }
};

PuzzleState::PuzzleState()
    : current_level_(0), moves_(0),
      phase_(PuzzlePhase::PLAYING), step_(PhaseStep::OPENING) {
}

void PuzzleState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);

    phase_ = PuzzlePhase::PLAYING;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void PuzzleState::update(StateManager& sm, SharedContext& ctx) {
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void PuzzleState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    bg_.reset();
    bg_map_.reset();
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}

void PuzzleState::change_phase(PuzzlePhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    phase_ = next;

    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void PuzzleState::enter_playing() {
    step_ = PhaseStep::RUNNING;
    // level_init();
}

void PuzzleState::update_playing(StateManager& sm, SharedContext& /*ctx*/) {
    if (bn::keypad::b_pressed()) {
        sm.change_state(StateID::MENU);
    }

    // if (clear) { change_phase(PuzzlePhase::CLEARED); }
}

void PuzzleState::exit_playing() {}

void PuzzleState::enter_cleared() {
    step_ = PhaseStep::RUNNING;
    // draw_clear_text(ctx);
}

void PuzzleState::update_cleared(StateManager& sm, SharedContext& /*ctx*/) {
    if (bn::keypad::a_pressed() || bn::keypad::b_pressed()) {
        sm.change_state(StateID::MENU);
    }
}

void PuzzleState::exit_cleared() {}

void PuzzleState::level_init() {
    // TODO
}

void PuzzleState::update_hud() {
    // TODO
}

void PuzzleState::draw_clear_text(SharedContext& /*ctx*/) {
    // TODO
}
