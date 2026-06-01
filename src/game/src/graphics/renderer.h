#ifndef RENDERER_H
#define RENDERER_H

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_array.h"

#include "game/puzzle_data.h"



class Renderer {
public:
    Renderer() = default;

    // BGマップセルへのポインタを受け取る
    void init(bn::array<bn::regular_bg_map_cell, 64 * 64>* map_cells);

    // PuzzleDataを元にBGマップを描画
    void render(const PuzzleData& data);

private:
    bn::array<bn::regular_bg_map_cell, 64 * 64>* map_cells_ = nullptr;
};

// ユーティリティ: PuzzleDataから直接 map_cells を構築して bg_map に反映する関数
void render_draw_map(bn::array<bn::regular_bg_map_cell, 64 * 64>& map_cells,
                     bn::regular_bg_map_ptr& bg_map, const PuzzleData& data);

#endif // RENDERER_H
