#include "renderer.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_item.h"
#include "bn_size.h"

void render_draw_map(bn::array<bn::regular_bg_map_cell, 32 * 32>& map_cells,
                     bn::regular_bg_map_ptr& bg_map, const GameState& gs) {
    for (int i = 0; i < 32 * 32; i++) {
        map_cells[i] = TILE_EMPTY;
    }

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            int tx = x + MAP_OFFSET_X;
            int ty = y + MAP_OFFSET_Y;
            map_cells[ty * 32 + tx] = gs.map[y][x];
        }
    }

    bg_map.set_cells_ref(bn::regular_bg_map_item(map_cells[0], bn::size(32, 32)));
    bg_map.reload_cells_ref();
}
