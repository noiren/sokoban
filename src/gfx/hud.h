#ifndef HUD_H
#define HUD_H

#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"
#include "bn_string.h"

class Hud {
public:
    void init(bn::sprite_text_generator& text_gen);
    void draw_game_hud(int moves, int stage);
    void draw_clear_message(int stage);
    void clear();

private:
    bn::sprite_text_generator* text_gen_ = nullptr;
    bn::vector<bn::sprite_ptr, 32> sprites_;
};

#endif // HUD_H
