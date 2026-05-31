#include "sokoban.h"
#include "levels.h"
#include "bn_memory.h"
#include "bn_math.h"

int get_num_levels() {
    return NUM_LEVELS;
}

// Check if all floor switches are pressed by barrels
static bool are_switches_pressed(const GameState& gs) {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (gs.bg_map[y][x] == BG_TILE_SWITCH) {
                if (gs.fg_map[y][x] != FG_OBJ_BARREL) {
                    return false;
                }
            }
        }
    }
    return true;
}

void game_init(GameState& gs, int level) {
    if (level < 0) level = 0;
    if (level >= NUM_LEVELS) level = NUM_LEVELS - 1;

    gs.current_level = level;
    gs.moves         = 0;
    gs.dropped_barrels = 0;
    gs.cleared       = false;
    gs.failed        = false;

    // Fill with EMPTY
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            gs.bg_map[y][x] = (unsigned char)BgTile::EMPTY;
            gs.fg_map[y][x] = (unsigned char)FgObj::NONE;
        }
    }

    // Load static 15x10 maps into the center of the 32x32 map
    int offset_x = (MAP_W - 15) / 2;
    int offset_y = (MAP_H - 10) / 2;

    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 15; x++) {
            gs.bg_map[y + offset_y][x + offset_x] = level_bg_data[level][y][x];
            gs.fg_map[y + offset_y][x + offset_x] = level_fg_data[level][y][x];
        }
    }

    gs.player_x = level_player_x[level] + offset_x;
    gs.player_y = level_player_y[level] + offset_y;
    gs.shady_x  = level_shady_x[level] == -1 ? -1 : level_shady_x[level] + offset_x;
    gs.shady_y  = level_shady_y[level] == -1 ? -1 : level_shady_y[level] + offset_y;


}

// Find warp counterpart
static void find_warp_destination(const GameState& gs, int src_x, int src_y, int warp_tile, int& dst_x, int& dst_y) {
    int target_warp = -1;
    if (warp_tile == BG_TILE_WARP_RED_A) target_warp = BG_TILE_WARP_RED_B;
    else if (warp_tile == BG_TILE_WARP_RED_B) target_warp = BG_TILE_WARP_RED_A;
    else if (warp_tile == BG_TILE_WARP_BLUE_A) target_warp = BG_TILE_WARP_BLUE_B;
    else if (warp_tile == BG_TILE_WARP_BLUE_B) target_warp = BG_TILE_WARP_BLUE_A;

    if (target_warp == -1) {
        dst_x = src_x;
        dst_y = src_y;
        return;
    }

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if ((y != src_y || x != src_x) && gs.bg_map[y][x] == target_warp) {
                dst_x = x;
                dst_y = y;
                return;
            }
        }
    }
    dst_x = src_x;
    dst_y = src_y;
}

// Rotates all arrow panels clockwise: UP -> RIGHT -> DOWN -> LEFT -> UP
static void rotate_arrows(GameState& gs) {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            unsigned char bg = gs.bg_map[y][x];
            if (bg == BG_TILE_ARROW_UP) gs.bg_map[y][x] = BG_TILE_ARROW_RIGHT;
            else if (bg == BG_TILE_ARROW_RIGHT) gs.bg_map[y][x] = BG_TILE_ARROW_DOWN;
            else if (bg == BG_TILE_ARROW_DOWN) gs.bg_map[y][x] = BG_TILE_ARROW_LEFT;
            else if (bg == BG_TILE_ARROW_LEFT) gs.bg_map[y][x] = BG_TILE_ARROW_UP;
        }
    }
}

// Helper to simulate sliding or arrow panel flows for any dynamic entity (Player or Barrel)
static void process_special_tiles(GameState& gs, int& cx, int& cy, int dx, int dy, unsigned char fg_id, bool switches_active) {
    bool moved = true;
    while (moved) {
        moved = false;

        // 1. Sliding on Ice
        if (gs.bg_map[cy][cx] == BG_TILE_ICE) {
            int nx = cx + dx;
            int ny = cy + dy;
            if (nx >= 0 && nx < MAP_W && ny >= 0 && ny < MAP_H) {
                if (gs.bg_map[ny][nx] != BG_TILE_WALL &&
                    (gs.bg_map[ny][nx] != BG_TILE_GOAL || switches_active) &&
                    gs.fg_map[ny][nx] == FG_OBJ_NONE) {
                    
                    gs.fg_map[cy][cx] = FG_OBJ_NONE;
                    cx = nx;
                    cy = ny;
                    gs.fg_map[cy][cx] = fg_id;
                    moved = true;
                    continue;
                }
            }
        }

        // 2. Forced Movement on Arrow Panels
        unsigned char tile = gs.bg_map[cy][cx];
        if (tile == BG_TILE_ARROW_UP || tile == BG_TILE_ARROW_DOWN ||
            tile == BG_TILE_ARROW_LEFT || tile == BG_TILE_ARROW_RIGHT) {
            
            int adx = 0, ady = 0;
            if (tile == BG_TILE_ARROW_UP) ady = -1;
            else if (tile == BG_TILE_ARROW_DOWN) ady = 1;
            else if (tile == BG_TILE_ARROW_LEFT) adx = -1;
            else if (tile == BG_TILE_ARROW_RIGHT) adx = 1;

            int nx = cx + adx;
            int ny = cy + ady;
            if (nx >= 0 && nx < MAP_W && ny >= 0 && ny < MAP_H) {
                if (gs.bg_map[ny][nx] != BG_TILE_WALL &&
                    (gs.bg_map[ny][nx] != BG_TILE_GOAL || switches_active) &&
                    gs.fg_map[ny][nx] == FG_OBJ_NONE) {
                    
                    gs.fg_map[cy][cx] = FG_OBJ_NONE;
                    cx = nx;
                    cy = ny;
                    gs.fg_map[cy][cx] = fg_id;
                    // Update slide direction
                    dx = adx;
                    dy = ady;
                    moved = true;
                    continue;
                }
            }
        }

        // 3. Teleport on Warp Tiles
        if (tile == BG_TILE_WARP_RED_A || tile == BG_TILE_WARP_RED_B ||
            tile == BG_TILE_WARP_BLUE_A || tile == BG_TILE_WARP_BLUE_B) {
            
            int wx = cx, wy = cy;
            find_warp_destination(gs, cx, cy, tile, wx, wy);
            if ((wx != cx || wy != cy) && gs.fg_map[wy][wx] == FG_OBJ_NONE) {
                gs.fg_map[cy][cx] = FG_OBJ_NONE;
                cx = wx;
                cy = wy;
                gs.fg_map[cy][cx] = fg_id;
                moved = true;
                continue;
            }
        }
    }
}

// Shady escapes by choosing a tile adjacent to Shady that is farthest from player
static void move_shady(GameState& gs) {
    if (gs.shady_x == -1 || gs.shady_y == -1) return;

    int best_x = gs.shady_x;
    int best_y = gs.shady_y;
    int max_dist_sq = (gs.player_x - gs.shady_x) * (gs.player_x - gs.shady_x) +
                      (gs.player_y - gs.shady_y) * (gs.player_y - gs.shady_y);

    const int dxs[] = { 0, 0, -1, 1 };
    const int dys[] = { -1, 1, 0, 0 };

    bool switches_active = are_switches_pressed(gs);

    for (int i = 0; i < 4; i++) {
        int nx = gs.shady_x + dxs[i];
        int ny = gs.shady_y + dys[i];

        if (nx >= 0 && nx < MAP_W && ny >= 0 && ny < MAP_H) {
            if (gs.bg_map[ny][nx] != BG_TILE_WALL &&
                (gs.bg_map[ny][nx] != BG_TILE_GOAL || switches_active) &&
                gs.fg_map[ny][nx] == FG_OBJ_NONE) {
                
                int dist_sq = (gs.player_x - nx) * (gs.player_x - nx) +
                              (gs.player_y - ny) * (gs.player_y - ny);
                if (dist_sq > max_dist_sq) {
                    max_dist_sq = dist_sq;
                    best_x = nx;
                    best_y = ny;
                }
            }
        }
    }

    if (best_x != gs.shady_x || best_y != gs.shady_y) {
        gs.fg_map[gs.shady_y][gs.shady_x] = FG_OBJ_NONE;
        gs.shady_x = best_x;
        gs.shady_y = best_y;
        gs.fg_map[gs.shady_y][gs.shady_x] = FG_OBJ_SHADY;

        // Process slides, arrows, and warps for Shady
        process_special_tiles(gs, gs.shady_x, gs.shady_y, best_x - gs.shady_x, best_y - gs.shady_y, FG_OBJ_SHADY, switches_active);
    }
}

void game_move(GameState& gs, int dx, int dy) {
    if (gs.cleared || gs.failed) return;

    int nx = gs.player_x + dx;
    int ny = gs.player_y + dy;

    if (nx < 0 || nx >= MAP_W || ny < 0 || ny >= MAP_H) return;

    bool switches_active = are_switches_pressed(gs);

    unsigned char dest_bg = gs.bg_map[ny][nx];
    if (dest_bg == BG_TILE_WALL) return;
    if (dest_bg == BG_TILE_GOAL && !switches_active) return; // Locked goal

    unsigned char dest_fg = gs.fg_map[ny][nx];

    // Push Barrel
    if (dest_fg == FG_OBJ_BARREL) {
        int bx = nx + dx;
        int by = ny + dy;

        if (bx < 0 || bx >= MAP_W || by < 0 || by >= MAP_H) return;

        unsigned char bdest_bg = gs.bg_map[by][bx];
        unsigned char bdest_fg = gs.fg_map[by][bx];

        if (bdest_bg == BG_TILE_WALL || (bdest_bg == BG_TILE_GOAL && !switches_active) || bdest_fg != FG_OBJ_NONE) return;

        // Perform barrel move
        gs.fg_map[ny][nx] = FG_OBJ_NONE;

        if (bdest_bg == BG_TILE_HOLE) {
            // Drop barrel in hole to fill it!
            gs.bg_map[by][bx] = BG_TILE_FLOOR;
            gs.dropped_barrels++;
        } else {
            gs.fg_map[by][bx] = FG_OBJ_BARREL;
            int b_cx = bx, b_cy = by;
            process_special_tiles(gs, b_cx, b_cy, dx, dy, FG_OBJ_BARREL, switches_active);
        }
    }

    // Perform player move
    gs.fg_map[gs.player_y][gs.player_x] = FG_OBJ_NONE;
    gs.player_x = nx;
    gs.player_y = ny;
    gs.fg_map[gs.player_y][gs.player_x] = FG_OBJ_PLAYER;
    gs.moves++;

    // Cracked Floor Degradation
    unsigned char player_bg = gs.bg_map[gs.player_y][gs.player_x];
    if (player_bg == BG_TILE_CRACKED_1) {
        gs.bg_map[gs.player_y][gs.player_x] = BG_TILE_CRACKED_2;
    } else if (player_bg == BG_TILE_CRACKED_2) {
        gs.bg_map[gs.player_y][gs.player_x] = BG_TILE_HOLE;
    } else if (player_bg == BG_TILE_HOLE) {
        gs.failed = true;
        return;
    }

    // Toggle switch rotation
    if (player_bg == BG_TILE_TOGGLE_SWITCH) {
        rotate_arrows(gs);
    }

    // Process slides, arrows, and warps for player
    process_special_tiles(gs, gs.player_x, gs.player_y, dx, dy, FG_OBJ_PLAYER, switches_active);

    // If player slides into a hole
    if (gs.bg_map[gs.player_y][gs.player_x] == BG_TILE_HOLE) {
        gs.failed = true;
        return;
    }

    // AI movement for Shady in Boss Stage
    if (gs.shady_x != -1 && gs.shady_y != -1) {
        move_shady(gs);
    }

    if (game_is_cleared(gs)) {
        gs.cleared = true;
    }
}

bool game_is_cleared(const GameState& gs) {
    if (gs.failed) return false;

    // Boss Stage Clear: Player is adjacent to Shady
    if (gs.shady_x != -1 && gs.shady_y != -1) {
        int dx = bn::abs(gs.player_x - gs.shady_x);
        int dy = bn::abs(gs.player_y - gs.shady_y);
        return (dx + dy <= 1);
    }

    // Normal Stage Clear: Player has reached the open Goal door
    if (gs.bg_map[gs.player_y][gs.player_x] == BG_TILE_GOAL) {
        return are_switches_pressed(gs);
    }

    return false;
}
