#include "renderer.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_item.h"
#include "bn_size.h"

void render_draw_map(bn::array<bn::regular_bg_map_cell, 32 * 32>& map_cells,
                     bn::regular_bg_map_ptr& bg_map, const PuzzleData& data) {
    // 全マスを EMPTY で初期化
    for (int i = 0; i < 32 * 32; i++) {
        map_cells[i] = (int)BgTile::EMPTY; // 17
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

    // BG + FG の2レイヤーを合成してタイルインデックスに変換する
    // タイルシート上のインデックス:
    //  0: 床（Floor）    1: 壁（Wall）   2: プレイヤー
    //  3: 樽（Barrel）   4: ゴール/スイッチ  5: 樽＋スイッチ
    //  6: シェイディ    7: 穴（Hole）   17: EMPTY
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
                    case BgTile::WALL:          render_tile = 1; break;
                    case BgTile::GOAL:          render_tile = switches_active ? 4 : 1; break;
                    case BgTile::SWITCH:        render_tile = 4; break;
                    case BgTile::HOLE:          render_tile = 7; break;
                    case BgTile::EXIT:          render_tile = 8; break; // 新設: 出口
                    case BgTile::CRACKED_2:     render_tile = 4; break; // TODO: 専用グラフィック
                    case BgTile::TOGGLE_SWITCH: render_tile = 4; break; // TODO: 専用グラフィック
                    default:                    render_tile = 0; break;
                }
            }

            map_cells[y * 32 + x] = render_tile;
        }
    }

    bg_map.set_cells_ref(bn::regular_bg_map_item(map_cells[0], bn::size(32, 32)));
    bg_map.reload_cells_ref();
}
