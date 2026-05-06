#include "save_select_state.h"
#include "state/Manager/state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "ui_data_save_select.h"

const SaveSelectState::PhaseHandlers SaveSelectState::phase_table_[] = {
    // SELECT_SLOT
    { &SaveSelectState::enter_select, &SaveSelectState::update_select, &SaveSelectState::exit_select }
};

SaveSelectState::SaveSelectState()
    : cursor_(0), selected_slot_(-1),
      phase_(SaveSelectPhase::SELECT_SLOT), step_(PhaseStep::OPENING) {
}

void SaveSelectState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    ui_.emplace(*ui_manager_);
    
    phase_ = SaveSelectPhase::SELECT_SLOT;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void SaveSelectState::update(StateManager& sm, SharedContext& ctx) {
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void SaveSelectState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    ui_.reset();
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}

void SaveSelectState::change_phase(SaveSelectPhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    phase_ = next;

    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void SaveSelectState::enter_select() {
    ui_manager_->load_screen(ui_data_save_select::SCREEN);
    cursor_ = 0;
    selected_slot_ = -1;
    step_ = PhaseStep::RUNNING;
    // fade_.start_fade_in(FADE_FRAMES); // 必要な場合は実装
}

void SaveSelectState::update_select(StateManager& sm, SharedContext& ctx) {
    // 最初の1フレーム等でUIの初期化を行いたい場合はここで update_slots_ui(ctx); を呼ぶ
    update_slots_ui(ctx);

    if (bn::keypad::up_pressed()) {
        cursor_--;
        if (cursor_ < 0) cursor_ = NUM_SAVE_SLOTS - 1;
    }

    if (bn::keypad::down_pressed()) {
        cursor_++;
        if (cursor_ >= NUM_SAVE_SLOTS) cursor_ = 0;
    }

    if (bn::keypad::a_pressed()) {
        SaveSlot& slot = ctx.save->slots[cursor_];
        if (!save_slot_is_valid(slot)) {
            save_slot_init(slot);
        }
        ctx.active_slot = cursor_;
        selected_slot_ = cursor_;
        sm.change_state(StateID::MENU);
    }

    if (bn::keypad::b_pressed()) {
        sm.change_state(StateID::TITLE);
    }
}

void SaveSelectState::exit_select() {}

void SaveSelectState::update_slots_ui(SharedContext& ctx) {
    if (!ui_.has_value()) return;

    for (int i = 0; i < NUM_SAVE_SLOTS; i++) {
        bool valid = save_slot_is_valid(ctx.save->slots[i]);
        bn::string<32> text;
        if (i == cursor_) text.append("> ");

        if (valid) {
            text.append("SLOT ");
            text.append(bn::to_string<4>(i + 1));
            text.append(" (");
            text.append(bn::to_string<4>(ctx.save->slots[i].story_chapter));
            text.append(")");
        } else {
            text.append("SLOT ");
            text.append(bn::to_string<4>(i + 1));
            text.append(" - NEW");
        }

        ui_.value().set_slot_text(i, text);
    }
}
