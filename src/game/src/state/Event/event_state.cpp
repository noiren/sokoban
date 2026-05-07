#include "event_state.h"
#include "state/Manager/state_manager.h"
#include "input/input_manager.h"

const EventState::PhaseHandlers EventState::phase_table_[] = {
    // EXECUTING
    { &EventState::enter_executing, &EventState::update_executing, &EventState::exit_executing },
    // WAITING_INPUT
    { &EventState::enter_waiting, &EventState::update_waiting, &EventState::exit_waiting },
    // FINISHED
    { &EventState::enter_finished, &EventState::update_finished, &EventState::exit_finished }
};

EventState::EventState()
    : script_(nullptr), pc_(0), phase_(EventPhase::EXECUTING),
      wants_puzzle_(false), puzzle_level_(0),
      text_char_index_(0), text_timer_(0), current_text_(nullptr),
      left_char_id_(-1), right_char_id_(-1), step_(PhaseStep::OPENING) {
}

void EventState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    ui_.emplace(*ui_manager_);
    
    pc_ = 0;
    wants_puzzle_ = false;

    // TODO: ctxなどから script_ を取得
    // script_ = ctx.next_event_script;

    phase_ = EventPhase::EXECUTING;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void EventState::update(StateManager& sm, SharedContext& ctx) {
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void EventState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    ui_.reset();
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}

void EventState::change_phase(EventPhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    phase_ = next;

    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void EventState::enter_executing() {
    step_ = PhaseStep::RUNNING;
}

void EventState::update_executing(StateManager& /*sm*/, SharedContext& ctx) {
    execute_next(ctx);
}

void EventState::exit_executing() {}

void EventState::enter_waiting() {
    step_ = PhaseStep::RUNNING;
}

void EventState::update_waiting(StateManager& /*sm*/, SharedContext& ctx) {
    update_dialog_text(ctx);
    if (InputManager::instance().is_triggered(Action::Decide)) {
        change_phase(EventPhase::EXECUTING);
    }
}

void EventState::exit_waiting() {}

void EventState::enter_finished() {
    step_ = PhaseStep::RUNNING;
}

void EventState::update_finished(StateManager& sm, SharedContext& /*ctx*/) {
    if (wants_puzzle_) {
        // sm.change_state(StateID::PUZZLE); // TODO
    } else {
        sm.change_state(StateID::MENU);
    }
}

void EventState::exit_finished() {}

void EventState::execute_next(SharedContext& /*ctx*/) {
    // TODO: スクリプト実行処理
    // 命令に応じて change_phase(EventPhase::WAITING_INPUT) または change_phase(EventPhase::FINISHED) に遷移
}

void EventState::update_dialog_text(SharedContext& /*ctx*/) {
    // TODO: テキストのタイプリターアニメーションなど
}

void EventState::clear_all() {
    // TODO: UIのクリア処理
}
