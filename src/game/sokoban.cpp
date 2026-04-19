#include "sokoban.h"
#include "levels.h"
#include "bn_memory.h"

int get_num_levels() {
    return NUM_LEVELS;
}

void game_init(GameState& gs, int level) {
    if (level < 0) level = 0;
    if (level >= NUM_LEVELS) level = NUM_LEVELS - 1;

    gs.current_level = level;
    gs.moves         = 0;
    gs.cleared       = false;

    bn::memory::copy(level_data[level][0][0], MAP_H * MAP_W, gs.map[0][0]);

    gs.player_x = level_player_x[level];
    gs.player_y = level_player_y[level];
}

void game_move(GameState& gs, int dx, int dy) {
    if (gs.cleared) return;

    int nx = gs.player_x + dx;
    int ny = gs.player_y + dy;

    if (nx < 0 || nx >= MAP_W || ny < 0 || ny >= MAP_H) return;

    unsigned char dest = gs.map[ny][nx];
    if (dest == TILE_WALL) return;

    if (dest == TILE_BOX || dest == TILE_BOX_ON_GOAL) {
        int bx = nx + dx;
        int by = ny + dy;
        if (bx < 0 || bx >= MAP_W || by < 0 || by >= MAP_H) return;

        unsigned char bdest = gs.map[by][bx];
        if (bdest == TILE_WALL || bdest == TILE_BOX || bdest == TILE_BOX_ON_GOAL) return;

        gs.map[by][bx] = (bdest == TILE_GOAL) ? TILE_BOX_ON_GOAL : TILE_BOX;
        gs.map[ny][nx] = (dest == TILE_BOX_ON_GOAL) ? TILE_GOAL : TILE_FLOOR;
    }

    unsigned char prev = gs.map[gs.player_y][gs.player_x];
    gs.map[gs.player_y][gs.player_x] = (prev == TILE_PLAYER_ON_GOAL) ? TILE_GOAL : TILE_FLOOR;

    unsigned char new_tile = gs.map[ny][nx];
    gs.map[ny][nx] = (new_tile == TILE_GOAL) ? TILE_PLAYER_ON_GOAL : TILE_PLAYER;

    gs.player_x = nx;
    gs.player_y = ny;
    gs.moves++;

    if (game_is_cleared(gs)) {
        gs.cleared = true;
    }
}

bool game_is_cleared(const GameState& gs) {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (gs.map[y][x] == TILE_BOX) {
                return false;
            }
        }
    }
    return true;
}
