#include "gallery_state.h"
#include "state/Manager/state_manager.h"
#include "input/input_manager.h"
#include "ui_data_gallerymenu.h"

GalleryState::GalleryState()
    : step_(PhaseStep::OPENING) {
}

void GalleryState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_gallerymenu::SCREEN);
    step_ = PhaseStep::RUNNING;
}

void GalleryState::update(StateManager& sm, SharedContext& ctx) {
    switch (step_) {
        case PhaseStep::OPENING:
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            update_browse(sm, ctx);
            break;

        case PhaseStep::CLOSING:
            break;
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void GalleryState::update_browse(StateManager& sm, SharedContext& /*ctx*/) {
    auto& inp = InputManager::instance();
    if (inp.is_triggered(Action::Cancel) || inp.is_triggered(Action::Decide)) {
        sm.change_state(StateID::MENU);
    }
}

void GalleryState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}
