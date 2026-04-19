#ifndef RENDERER_H
#define RENDERER_H

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_array.h"

#include "game/sokoban.h"

// Map rendering offsets to center 15x10 on 30x20 screen
constexpr int MAP_OFFSET_X = (30 - MAP_W) / 2;
constexpr int MAP_OFFSET_Y = (20 - MAP_H) / 2;

class Renderer {
public:
    void init();
    void draw_map(const GameState& gs);
    void clear_screen();

private:
    bn::regular_bg_ptr* bg_ = nullptr;
    bn::regular_bg_map_ptr* bg_map_ = nullptr;
    bn::array<bn::regular_bg_map_cell, 32 * 32>* map_cells_ = nullptr;
};

// Standalone functions (used before Renderer is fully set up)
void render_draw_map(bn::array<bn::regular_bg_map_cell, 32 * 32>& map_cells,
                     bn::regular_bg_map_ptr& bg_map, const GameState& gs);

#endif // RENDERER_H
