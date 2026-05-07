#ifndef HUD_H
#define HUD_H

#include "bn_string.h"
#include "ui_manager.h"

class Hud {
public:
    void init(UIManager& ui_manager);
    void draw_game_hud(int moves, int stage);
    void draw_clear_message(int stage);
    void set_level(int stage);
    void clear();

private:
    UIManager* ui_ = nullptr;
    int stage_ = 0;
};

#endif // HUD_H
