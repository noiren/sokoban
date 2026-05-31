#ifndef TILE_HANDLER_H
#define TILE_HANDLER_H

#include "game/puzzle_types.h"

class PuzzleEngine; // 前方宣言

// --- タイルの振る舞いハンドラ ---
// タイル種別ごとに、「乗った瞬間の効果」と「連続移動の方向計算」を持つ。
// テーブル（配列）として定義し、巨大switch文を排除する。
struct TileHandler {
    // プレイヤーや樽がそのマスに到達した瞬間に呼ばれる。
    // nullptr の場合は何もしない（通常床など）。
    void (*on_enter)(PuzzleEngine& engine, int x, int y, FgObj obj);

    // 連続移動（氷・矢印）の次の移動方向を計算する。
    // 戻り値が true なら連続移動を継続、false なら停止。
    // dx, dy は入力としても出力としても使われる（矢印は強制上書き）。
    // nullptr の場合は停止（通常床、壁など）。
    bool (*get_next_move)(const PuzzleEngine& engine, int x, int y, int& dx, int& dy);
};

// ハンドラテーブルの外部宣言（tile_handlers.cpp で定義）
extern const TileHandler TILE_HANDLERS[(int)BgTile::EMPTY + 1];

#endif // TILE_HANDLER_H
