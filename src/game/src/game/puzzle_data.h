#ifndef PUZZLE_DATA_H
#define PUZZLE_DATA_H

#include "game/puzzle_types.h"
#include "bn_array.h"

// --- 盤面の純粋なデータコンテナ ---
// ゲームの状態はすべてこの構造体に収まる。
// Undo機能やリトライのためのスナップショットも、この構造体をコピーするだけ。
struct PuzzleData {
    bn::array<bn::array<BgTile, MAP_W>, MAP_H> bg_map;
    bn::array<bn::array<FgObj,  MAP_W>, MAP_H> fg_map;

    int player_x       = 0;
    int player_y       = 0;
    int shady_x        = -1; // -1 のときシェイディは存在しない（通常ステージ）
    int shady_y        = -1;
    int moves          = 0;
    int dropped_barrels = 0; // 穴に落とした樽の数（やり込み判定用）
};

#endif // PUZZLE_DATA_H
