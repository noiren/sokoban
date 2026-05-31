#ifndef RENDERER_H
#define RENDERER_H

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_array.h"

#include "game/puzzle_data.h"



class Renderer {
public:
    void init();
    void draw_map(const PuzzleData& data);
    void clear_screen();

private:
    bn::regular_bg_ptr* bg_ = nullptr;
    bn::regular_bg_map_ptr* bg_map_ = nullptr;
    bn::array<bn::regular_bg_map_cell, 32 * 32>* map_cells_ = nullptr;
};

// Standalone functions (used before Renderer is fully set up)
void render_draw_map(bn::array<bn::regular_bg_map_cell, 32 * 32>& map_cells,
                     bn::regular_bg_map_ptr& bg_map, const PuzzleData& data);

#endif // RENDERER_H
