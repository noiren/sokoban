#include "renderer.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_item.h"
#include "bn_size.h"
#include "bn_regular_bg_items_still_puzzle_map.h"

void render_draw_map(bn::array<bn::regular_bg_map_cell, 64 * 64>& map_cells,
                     bn::regular_bg_map_ptr& bg_map, const PuzzleData& data) {
    // 全マスを FLOOR (0) で初期化
    for (int i = 0; i < 64 * 64; i++) {
        map_cells[i] = 0;
    }

    // スイッチが全部押されているか確認（ゴールの表示切り替えに使う）
    bool switches_active = true;
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (data.bg_map[y][x] == BgTile::SWITCH &&
                data.fg_map[y][x] != FgObj::BARREL) {
                switches_active = false;
            }
        }
    }

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {

            FgObj fg = data.fg_map[y][x];
            BgTile bg = data.bg_map[y][x];

            int render_tile = 0;

            if (fg == FgObj::PLAYER) {
                render_tile = 2;
            } else if (fg == FgObj::BARREL) {
                render_tile = (bg == BgTile::SWITCH) ? 5 : 3;
            } else if (fg == FgObj::SHADY) {
                render_tile = 6;
            } else {
                switch (bg) {
                    case BgTile::FLOOR:         render_tile = 0; break;
                    case BgTile::WALL:          render_tile = 1; break;
                    case BgTile::GOAL:          render_tile = switches_active ? 22 : 21; break;
                    case BgTile::SWITCH:        render_tile = 4; break;
                    case BgTile::HOLE:          render_tile = 7; break;
                    case BgTile::EXIT:          render_tile = 8; break;
                    case BgTile::ICE:           render_tile = 9; break;
                    case BgTile::CRACKED_1:     render_tile = 10; break;
                    case BgTile::CRACKED_2:     render_tile = 11; break;
                    case BgTile::ARROW_UP:      render_tile = 12; break;
                    case BgTile::ARROW_DOWN:    render_tile = 13; break;
                    case BgTile::ARROW_LEFT:    render_tile = 14; break;
                    case BgTile::ARROW_RIGHT:   render_tile = 15; break;
                    case BgTile::TOGGLE_SWITCH: render_tile = 16; break;
                    case BgTile::WARP_RED_A:    render_tile = 17; break;
                    case BgTile::WARP_RED_B:    render_tile = 18; break;
                    case BgTile::WARP_BLUE_A:   render_tile = 19; break;
                    case BgTile::WARP_BLUE_B:   render_tile = 20; break;
                    case BgTile::EMPTY:         render_tile = 0; break; // EMPTY mapped to Floor
                    default:                    render_tile = 0; break;
                }
            }

            int map_base_x = (render_tile % 16) * 2;
            int map_base_y = (render_tile / 16) * 2;
            
            const auto* src_cells = &bn::regular_bg_items::still_puzzle_map.map_item().cells_ref();
            
            auto get_map_index_64x64 = [](int cx, int cy) {
                int sbb_x = cx / 32;
                int sbb_y = cy / 32;
                int sbb_idx = sbb_y * 2 + sbb_x;
                int local_x = cx % 32;
                int local_y = cy % 32;
                return sbb_idx * 1024 + local_y * 32 + local_x;
            };

            int hw_tl = src_cells[get_map_index_64x64(map_base_x, map_base_y)];
            int hw_tr = src_cells[get_map_index_64x64(map_base_x + 1, map_base_y)];
            int hw_bl = src_cells[get_map_index_64x64(map_base_x, map_base_y + 1)];
            int hw_br = src_cells[get_map_index_64x64(map_base_x + 1, map_base_y + 1)];
            
            int out_x = x * 2;
            int out_y = y * 2;
            map_cells[get_map_index_64x64(out_x, out_y)] = hw_tl;
            map_cells[get_map_index_64x64(out_x + 1, out_y)] = hw_tr;
            map_cells[get_map_index_64x64(out_x, out_y + 1)] = hw_bl;
            map_cells[get_map_index_64x64(out_x + 1, out_y + 1)] = hw_br;
        }
    }

    bg_map.set_cells_ref(bn::regular_bg_map_item(map_cells[0], bn::size(64, 64)));
    bg_map.reload_cells_ref();
}
