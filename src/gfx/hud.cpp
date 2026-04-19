#include "hud.h"

void Hud::init(bn::sprite_text_generator& text_gen) {
    text_gen_ = &text_gen;
}

void Hud::draw_game_hud(int moves, int stage) {
    sprites_.clear();

    text_gen_->set_left_alignment();
    bn::string<32> moves_text = "MOVES:";
    moves_text.append(bn::to_string<8>(moves));
    text_gen_->generate(-120 + 8, 80 - 12, moves_text, sprites_);

    text_gen_->set_right_alignment();
    bn::string<32> stage_text = "STAGE:";
    stage_text.append(bn::to_string<4>(stage));
    text_gen_->generate(120 - 8, 80 - 12, stage_text, sprites_);
}

void Hud::draw_clear_message(int stage) {
    sprites_.clear();

    text_gen_->set_center_alignment();
    bn::string<32> clear_text = "STAGE ";
    clear_text.append(bn::to_string<4>(stage));
    clear_text.append(" CLEAR!");
    text_gen_->generate(0, -16, clear_text, sprites_);
    text_gen_->generate(0, 0, "PRESS START", sprites_);
}

void Hud::clear() {
    sprites_.clear();
}
