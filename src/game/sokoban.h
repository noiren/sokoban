#ifndef SOKOBAN_H
#define SOKOBAN_H

// --- Map constants ---
#define MAP_W   15
#define MAP_H   10

// --- Tile IDs ---
#define TILE_FLOOR         0
#define TILE_WALL          1
#define TILE_PLAYER        2
#define TILE_BOX           3
#define TILE_GOAL          4
#define TILE_BOX_ON_GOAL   5
#define TILE_PLAYER_ON_GOAL 6
#define TILE_EMPTY         7

int get_num_levels();

struct GameState {
    unsigned char map[MAP_H][MAP_W];
    int player_x;
    int player_y;
    int moves;
    int current_level;
    bool cleared;
};

void game_init(GameState& gs, int level);
void game_move(GameState& gs, int dx, int dy);
bool game_is_cleared(const GameState& gs);

#endif // SOKOBAN_H
