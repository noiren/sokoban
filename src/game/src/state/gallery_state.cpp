#include "gallery_state.h"
#include "state_manager.h"
#include "bn_keypad.h"

GalleryState::GalleryState(bn::sprite_text_generator& text_gen)
    : text_gen_(text_gen) {
}

void GalleryState::init(StateManager& /*manager*/) {
    sprites_.clear();
    text_gen_.set_center_alignment();
    text_gen_.generate(0, -8, "GALLERY",        sprites_);
    text_gen_.generate(0,  8, "Coming Soon...", sprites_);
}

void GalleryState::update(StateManager& manager) {
    if (bn::keypad::b_pressed() || bn::keypad::a_pressed()) {
        manager.pop();
    }
}

void GalleryState::shutdown() {
    sprites_.clear();
}
