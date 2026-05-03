#include "hud.h"

#include "hud.h"

void Hud::init(UIManager& ui_manager) {
    ui_ = &ui_manager;
}

void Hud::draw_game_hud(int moves, int stage) {
    if (!ui_) return;

    bn::string<32> moves_text = "MOVES:";
    moves_text.append(bn::to_string<8>(moves));
    ui_->set_text("moves_text", moves_text);

    bn::string<32> stage_text = "STAGE:";
    stage_text.append(bn::to_string<4>(stage));
    ui_->set_text("stage_text", stage_text);
    
    // クリアメッセージは非表示にしておく
    ui_->set_text_visible("clear_text", false);
    ui_->set_text_visible("start_text", false);
}

void Hud::draw_clear_message(int stage) {
    if (!ui_) return;

    bn::string<32> clear_text = "STAGE ";
    clear_text.append(bn::to_string<4>(stage));
    clear_text.append(" CLEAR!");
    
    ui_->set_text("clear_text", clear_text);
    ui_->set_text_visible("clear_text", true);
    
    ui_->set_text("start_text", "PRESS START");
    ui_->set_text_visible("start_text", true);
}

void Hud::clear() {
    if (!ui_) return;
    ui_->set_text("moves_text", "");
    ui_->set_text("stage_text", "");
    ui_->set_text_visible("clear_text", false);
    ui_->set_text_visible("start_text", false);
}
