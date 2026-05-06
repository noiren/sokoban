#include "hud.h"

void Hud::init(UIManager& ui_manager) {
    ui_ = &ui_manager;
}

void Hud::set_level(int stage) {
    stage_ = stage;
}

void Hud::draw_game_hud(int moves, int stage) {
    if (!ui_) return;

    if (stage >= 0) stage_ = stage;

    bn::string<32> moves_text = "MOVES:";
    moves_text.append(bn::to_string<8>(moves));
    if (auto* node = ui_->get_text("moves_text")) {
        node->set_text(moves_text);
    }

    bn::string<32> stage_text = "STAGE:";
    stage_text.append(bn::to_string<4>(stage_ + 1));
    if (auto* node = ui_->get_text("stage_text")) {
        node->set_text(stage_text);
    }
    
    // クリアメッセージは非表示にしておく
    if (auto* node = ui_->get_text("clear_text")) {
        node->set_visible(false);
    }
    if (auto* node = ui_->get_text("start_text")) {
        node->set_visible(false);
    }
}

void Hud::draw_clear_message(int stage) {
    if (!ui_) return;

    bn::string<32> clear_text = "STAGE ";
    clear_text.append(bn::to_string<4>(stage));
    clear_text.append(" CLEAR!");
    
    if (auto* node = ui_->get_text("clear_text")) {
        node->set_text(clear_text);
        node->set_visible(true);
    }
    
    if (auto* node = ui_->get_text("start_text")) {
        node->set_text("PRESS START");
        node->set_visible(true);
    }
}

void Hud::clear() {
    if (!ui_) return;
    if (auto* node = ui_->get_text("moves_text")) {
        node->set_text("");
    }
    if (auto* node = ui_->get_text("stage_text")) {
        node->set_text("");
    }
    if (auto* node = ui_->get_text("clear_text")) {
        node->set_visible(false);
    }
    if (auto* node = ui_->get_text("start_text")) {
        node->set_visible(false);
    }
}
