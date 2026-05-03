#include "gallery_state.h"
#include "state_manager.h"
#include "bn_keypad.h"
#include "ui_data_gallerymenu.h"

GalleryState::GalleryState(bn::sprite_text_generator& text_gen)
    : ui_manager_(text_gen), step_(PhaseStep::OPENING) {
}

void GalleryState::init(StateManager& /*manager*/) {
    ui_manager_.load_screen(ui_data_gallerymenu::SCREEN);
    step_ = PhaseStep::OPENING;
    // TODO: OPENINGフェード等を追加する場合はここで設定
    step_ = PhaseStep::RUNNING;  // 現時点はフェードなしで即開始
}

void GalleryState::update(StateManager& manager) {
    switch (step_) {
        case PhaseStep::OPENING:
            // TODO: フェードイン処理
            step_ = PhaseStep::RUNNING;
            break;

        case PhaseStep::RUNNING:
            update_browse(manager);
            break;

        case PhaseStep::CLOSING:
            // TODO: フェードアウト処理
            manager.pop();
            break;
    }

    ui_manager_.update();
}

void GalleryState::update_browse(StateManager& /*manager*/) {
    if (bn::keypad::b_pressed() || bn::keypad::a_pressed()) {
        step_ = PhaseStep::CLOSING;
    }
}

void GalleryState::shutdown() {
    ui_manager_.clear_all();
}
