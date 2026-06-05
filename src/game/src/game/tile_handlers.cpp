#include "game/tile_handler.h"
#include "game/puzzle_engine.h"

// BgTile enum の値が TILE_HANDLERS テーブルのインデックスと一致していることを保証
static_assert((int)BgTile::FLOOR == 0, "BgTile enum changed: update TILE_HANDLERS!");
static_assert((int)BgTile::WALL == 1, "BgTile enum changed: update TILE_HANDLERS!");
static_assert((int)BgTile::HOLE == 7, "BgTile enum changed: update TILE_HANDLERS!");
static_assert((int)BgTile::EXIT == 8, "BgTile enum changed: update TILE_HANDLERS!");
static_assert((int)BgTile::WARP_BLUE_B == 17, "BgTile enum changed: update TILE_HANDLERS!");
static_assert((int)BgTile::EMPTY == 18, "BgTile enum changed: update TILE_HANDLERS!");

// =============================================================================
// ハンドラ関数の定義
// =============================================================================

// --- 床タイル：何もしない ---
// （FLOOR, WALL, GOAL, SWITCH など。ハンドラ不要なものは nullptr でよい。）

// --- ボロボロ床1（CRACKED_1）：踏んだら CRACKED_2 に変化 ---
static void cracked_1_on_enter(PuzzleEngine& engine, int x, int y, FgObj /*obj*/) {
    engine.change_bg(x, y, BgTile::CRACKED_2);
    engine.push_event({EventType::PLAY_SE, (uint8_t)x, (uint8_t)y, 0, 0, BgTile::FLOOR, 1}); // SE: ヒビ音
}

// --- ボロボロ床2（CRACKED_2）：踏んだら HOLE に変化 ---
static void cracked_2_on_enter(PuzzleEngine& engine, int x, int y, FgObj /*obj*/) {
    engine.change_bg(x, y, BgTile::HOLE);
    engine.push_event({EventType::PLAY_SE, (uint8_t)x, (uint8_t)y, 0, 0, BgTile::FLOOR, 2}); // SE: 崩れ音
}

// --- 氷（ICE）：現在の移動方向を維持して滑り続ける ---
// Engineの外側でis_passable等のチェックをするため、ここはtrueを返すだけ。
static bool ice_get_next_move(const PuzzleEngine& /*engine*/, int /*x*/, int /*y*/, int& /*dx*/, int& /*dy*/) {
    return true; // 方向はそのまま維持
}

// --- 矢印パネル（上）：上方向に強制移動 ---
static bool arrow_up_get_next_move(const PuzzleEngine& /*engine*/, int /*x*/, int /*y*/, int& dx, int& dy) {
    dx = 0; dy = -1;
    return true;
}

// --- 矢印パネル（下）：下方向に強制移動 ---
static bool arrow_down_get_next_move(const PuzzleEngine& /*engine*/, int /*x*/, int /*y*/, int& dx, int& dy) {
    dx = 0; dy = 1;
    return true;
}

// --- 矢印パネル（左）：左方向に強制移動 ---
static bool arrow_left_get_next_move(const PuzzleEngine& /*engine*/, int /*x*/, int /*y*/, int& dx, int& dy) {
    dx = -1; dy = 0;
    return true;
}

// --- 矢印パネル（右）：右方向に強制移動 ---
static bool arrow_right_get_next_move(const PuzzleEngine& /*engine*/, int /*x*/, int /*y*/, int& dx, int& dy) {
    dx = 1; dy = 0;
    return true;
}

// =============================================================================
// ハンドラテーブルの実体定義（BgTile の値と順番を必ず一致させること）
// =============================================================================
// static const にすることで、C++17のconstexpr制約（関数ポインタ非対応）を回避しつつ
// ROMデータ領域への配置でメモリオーバーヘッドを最小化する。
const TileHandler TILE_HANDLERS[(int)BgTile::EMPTY + 1] = {
    /* 0:  FLOOR         */ { nullptr,           nullptr                   },
    /* 1:  WALL          */ { nullptr,           nullptr                   },
    /* 2:  GOAL          */ { nullptr,           nullptr                   },
    /* 3:  SWITCH        */ { nullptr,           nullptr                   },
    /* 4:  ICE           */ { nullptr,           ice_get_next_move         },
    /* 5:  CRACKED_1     */ { cracked_1_on_enter, nullptr                  },
    /* 6:  CRACKED_2     */ { cracked_2_on_enter, nullptr                  },
    /* 7:  HOLE          */ { nullptr,           nullptr                   },
    /* 8:  EXIT          */ { nullptr,           nullptr                   },
    /* 9:  ARROW_UP      */ { nullptr,           arrow_up_get_next_move    },
    /* 10: ARROW_DOWN    */ { nullptr,           arrow_down_get_next_move  },
    /* 11: ARROW_LEFT    */ { nullptr,           arrow_left_get_next_move  },
    /* 12: ARROW_RIGHT   */ { nullptr,           arrow_right_get_next_move },
    /* 13: TOGGLE_SWITCH */ { nullptr,           nullptr                   },
    /* 14: WARP_RED_A    */ { nullptr,           nullptr                   },
    /* 15: WARP_RED_B    */ { nullptr,           nullptr                   },
    /* 16: WARP_BLUE_A   */ { nullptr,           nullptr                   },
    /* 17: WARP_BLUE_B   */ { nullptr,           nullptr                   },
};
