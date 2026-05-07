#include "endless_state.h"
#include "state/Manager/state_manager.h"
#include "input/input_manager.h"
#include "bn_string.h"

const EndlessState::PhaseHandlers EndlessState::phase_table_[] = {
    // PLAYING
    { &EndlessState::enter_playing, &EndlessState::update_playing, &EndlessState::exit_playing },
    // RESULT
    { &EndlessState::enter_result, &EndlessState::update_result, &EndlessState::exit_result }
};

EndlessState::EndlessState()
    : score_(0), difficulty_(1), seed_(0),
      phase_(EndlessPhase::PLAYING), step_(PhaseStep::OPENING) {
}

void EndlessState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    
    score_ = 0;
    difficulty_ = 1;
    // generate_next(); // TODO: 実装

    phase_ = EndlessPhase::PLAYING;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void EndlessState::update(StateManager& sm, SharedContext& ctx) {
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void EndlessState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    bg_.reset();
    bg_map_.reset();
    result_sprites_.clear();

    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}

void EndlessState::change_phase(EndlessPhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    phase_ = next;

    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void EndlessState::enter_playing() {
    step_ = PhaseStep::RUNNING;
}

void EndlessState::update_playing(StateManager& sm, SharedContext& /*ctx*/) {
    if (InputManager::instance().is_triggered(Action::Cancel)) {
        sm.change_state(StateID::MENU);
    }
    // TODO: 実際のパズル処理を呼び出し、クリアしたら次のレベル生成またはリザルトへ
    // if (clear) { score_++; generate_next(); }
    // if (game_over) { change_phase(EndlessPhase::RESULT); }
}

void EndlessState::exit_playing() {}

void EndlessState::enter_result() {
    step_ = PhaseStep::RUNNING;
    // draw_result(ctx);
}

void EndlessState::update_result(StateManager& sm, SharedContext& /*ctx*/) {
    if (InputManager::instance().is_triggered(Action::Decide) || InputManager::instance().is_triggered(Action::Cancel)) {
        sm.change_state(StateID::MENU);
    }
}

void EndlessState::exit_result() {}

void EndlessState::generate_next() {
    // TODO
}

void EndlessState::draw_result(SharedContext& /*ctx*/) {
    // TODO
}
