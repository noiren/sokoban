#ifndef SOKOBAN_H
#define SOKOBAN_H

// MAP_W and MAP_H are now defined in puzzle_types.h
// They are kept accessible via sokoban.h for legacy code that includes sokoban.h
#include "puzzle_types.h"

// --- Background Tile IDs ---
#define BG_TILE_FLOOR           0
#define BG_TILE_WALL            1
#define BG_TILE_GOAL            2
#define BG_TILE_SWITCH          3
#define BG_TILE_ICE             4
#define BG_TILE_CRACKED_1       5 // Untouched
#define BG_TILE_CRACKED_2       6 // Stepped once
#define BG_TILE_HOLE            7 // Pit/fell in
#define BG_TILE_ARROW_UP        8
#define BG_TILE_ARROW_DOWN      9
#define BG_TILE_ARROW_LEFT      10
#define BG_TILE_ARROW_RIGHT     11
#define BG_TILE_TOGGLE_SWITCH   12
#define BG_TILE_WARP_RED_A      13
#define BG_TILE_WARP_RED_B      14
#define BG_TILE_WARP_BLUE_A     15
#define BG_TILE_WARP_BLUE_B     16
#define BG_TILE_EMPTY           17

// --- Foreground Object IDs ---
#define FG_OBJ_NONE             0
#define FG_OBJ_PLAYER           1
#define FG_OBJ_BARREL           2
#define FG_OBJ_SHADY            3

// get_num_levels() は levels.h に移動しました

struct GameState {
    unsigned char bg_map[MAP_H][MAP_W];
    unsigned char fg_map[MAP_H][MAP_W];
    int player_x;
    int player_y;
    int shady_x;
    int shady_y;
    int moves;
    int current_level;
    int dropped_barrels;
    bool cleared;
    bool failed;
};

void game_init(GameState& gs, int level);
void game_move(GameState& gs, int dx, int dy);
bool game_is_cleared(const GameState& gs);

#endif // SOKOBAN_H
