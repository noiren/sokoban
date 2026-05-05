#include "save_select_state.h"
#include "state/Manager/state_manager.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "ui_data_save_select.h"

SaveSelectState::SaveSelectState()
    : cursor_(0), selected_slot_(-1),
      step_(PhaseStep::OPENING) {
}

void SaveSelectState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_save_select::SCREEN);
    ui_.emplace(*ui_manager_);
    cursor_ = 0;
    selected_slot_ = -1;
    update_slots_ui(ctx);
    step_ = PhaseStep::RUNNING;
}

void SaveSelectState::update(StateManager& sm, SharedContext& ctx) {
    switch (step_) {
        case PhaseStep::OPENING:
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            update_select(sm, ctx);
            break;

        case PhaseStep::CLOSING:
            break;
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void SaveSelectState::update_select(StateManager& sm, SharedContext& ctx) {
    if (bn::keypad::up_pressed()) {
        cursor_--;
        if (cursor_ < 0) cursor_ = NUM_SAVE_SLOTS - 1;
        update_slots_ui(ctx);
    }

    if (bn::keypad::down_pressed()) {
        cursor_++;
        if (cursor_ >= NUM_SAVE_SLOTS) cursor_ = 0;
        update_slots_ui(ctx);
    }

    if (bn::keypad::a_pressed()) {
        // スロットを選択→init済みならロード、未初期化なら新規
        SaveSlot& slot = ctx.save->slots[cursor_];
        if (!save_slot_is_valid(slot)) {
            save_slot_init(slot);
        }
        ctx.active_slot = cursor_;
        selected_slot_ = cursor_;
        step_ = PhaseStep::CLOSING;
        sm.change_state(StateID::MENU);
    }

    // B = タイトルに戻る
    if (bn::keypad::b_pressed()) {
        sm.change_state(StateID::TITLE);
    }
}

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

void SaveSelectState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    ui_.reset();
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}
