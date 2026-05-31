#ifndef PUZZLE_TYPES_H
#define PUZZLE_TYPES_H

#include "bn_fixed.h"

constexpr int MAP_W = 32;
constexpr int MAP_H = 32;

// --- 床タイルの種別 ---
enum class BgTile : uint8_t {
    FLOOR = 0,
    WALL,
    GOAL,
    SWITCH,
    ICE,
    CRACKED_1,      // ボロボロ床（未踏）
    CRACKED_2,      // ボロボロ床（ヒビ入り）
    HOLE,           // 穴（落下したらアウト）
    EXIT,           // 明確な出口
    ARROW_UP,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT,
    TOGGLE_SWITCH,  // 踏むと全矢印を時計回りに回転
    WARP_RED_A,
    WARP_RED_B,
    WARP_BLUE_A,
    WARP_BLUE_B,
    EMPTY = 17      // マップ外（描画用）
};

// --- 前景オブジェクトの種別 ---
enum class FgObj : uint8_t {
    NONE = 0,
    PLAYER,
    BARREL,
    SHADY
};

// --- エンジンから描画側へ渡すイベントの種別 ---
enum class EventType : uint8_t {
    MOVE_FG,    // 前景オブジェクトの移動
    CHANGE_BG,  // 床タイルの変化（ボロボロが崩れるなど）
    PLAY_SE     // 効果音再生
};

// --- イベント1件分 ---
struct PuzzleEvent {
    EventType type;
    uint8_t   x, y;        // 対象マスの座標（移動元 or 変更マス）
    int8_t    dx, dy;      // MOVE_FG 用の移動ベクトル（ワープは絶対座標で扱う）
    BgTile    new_bg;      // CHANGE_BG 用の新しいタイル種別
    uint8_t   se_id;       // PLAY_SE 用の効果音ID
};

#endif // PUZZLE_TYPES_H
