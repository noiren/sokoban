#include "puzzle_gen.h"
#include "bn_common.h"
#include "bn_assert.h"

// Simple LCG random number generator
static int rng_state = 1;

static void rng_seed(int seed) {
    rng_state = seed;
    if (rng_state == 0) rng_state = 1;
}

static int rng_next() {
    rng_state = rng_state * 1103515245 + 12345;
    return (rng_state >> 16) & 0x7FFF;
}

static int rng_range(int min_val, int max_val) {
    return min_val + (rng_next() % (max_val - min_val + 1));
}

// Check if position is valid for placement
static bool is_open(const GameState& gs, int x, int y) {
    if (x < 1 || x >= MAP_W - 1 || y < 1 || y >= MAP_H - 1) return false;
    return gs.bg_map[y][x] == BG_TILE_FLOOR && gs.fg_map[y][x] == FG_OBJ_NONE;
}

// Simple flood-fill reachability check
// NOTE: visited/stack はIWRAMスタックに収まらないため、EWRAMに静的確保する
static bool flood_check(const unsigned char map[MAP_H][MAP_W], int sx, int sy, int tx, int ty) {
    BN_DATA_EWRAM static bool visited[MAP_H][MAP_W];
    struct Pos { int x, y; };
    BN_DATA_EWRAM static Pos stack[MAP_W * MAP_H];

    // visited を初期化
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            visited[y][x] = false;
        }
    }

    int top = 0;

    BN_ASSERT(top < MAP_W * MAP_H, "puzzle_gen flood_check: stack overflow");
    stack[top++] = {sx, sy};
    visited[sy][sx] = true;

    while (top > 0) {
        Pos cur = stack[--top];
        if (cur.x == tx && cur.y == ty) return true;

        const int dx[] = {0, 0, -1, 1};
        const int dy[] = {-1, 1, 0, 0};
        for (int d = 0; d < 4; d++) {
            int nx = cur.x + dx[d];
            int ny = cur.y + dy[d];
            if (nx >= 0 && nx < MAP_W && ny >= 0 && ny < MAP_H &&
                !visited[ny][nx] && map[ny][nx] != BG_TILE_WALL) {
                visited[ny][nx] = true;
                BN_ASSERT(top < MAP_W * MAP_H, "puzzle_gen flood_check: stack overflow");
                stack[top++] = {nx, ny};
            }
        }
    }
    return false;
}

// Generate a puzzle by "reverse solving" using the new dual-layer representation
bool puzzle_generate(GameState& gs, int difficulty, int seed) {
    rng_seed(seed);

    int num_boxes = 1 + difficulty;
    if (num_boxes > 3) num_boxes = 3;

    // Step 1: Create room with random walls
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (x == 0 || x == MAP_W - 1 || y == 0 || y == MAP_H - 1) {
                gs.bg_map[y][x] = BG_TILE_WALL;
            } else {
                gs.bg_map[y][x] = BG_TILE_FLOOR;
            }
            gs.fg_map[y][x] = FG_OBJ_NONE;
        }
    }

    // Add some random interior walls
    int wall_count = rng_range(8, 18);
    for (int i = 0; i < wall_count; i++) {
        int wx = rng_range(2, MAP_W - 3);
        int wy = rng_range(2, MAP_H - 3);
        gs.bg_map[wy][wx] = BG_TILE_WALL;
    }

    // Step 2: Place goals and boxes
    struct Pos { int x, y; };
    Pos goals[3];
    Pos boxes[3];
    int placed = 0;

    for (int attempt = 0; attempt < 100 && placed < num_boxes; attempt++) {
        int gx = rng_range(2, MAP_W - 3);
        int gy = rng_range(2, MAP_H - 3);
        if (gs.bg_map[gy][gx] == BG_TILE_FLOOR) {
            bool ok = true;
            for (int j = 0; j < placed; j++) {
                int dx = gx - goals[j].x;
                int dy = gy - goals[j].y;
                if (dx * dx + dy * dy < 4) { ok = false; break; }
            }
            if (ok) {
                goals[placed] = {gx, gy};
                boxes[placed] = {gx, gy};
                gs.bg_map[gy][gx] = BG_TILE_SWITCH; // Switches act as goals
                gs.fg_map[gy][gx] = FG_OBJ_BARREL;
                placed++;
            }
        }
    }

    if (placed < num_boxes) return false;

    // Step 3: Place player
    int px = 0, py = 0;
    bool player_placed = false;
    for (int attempt = 0; attempt < 100; attempt++) {
        px = rng_range(1, MAP_W - 2);
        py = rng_range(1, MAP_H - 2);
        if (gs.bg_map[py][px] == BG_TILE_FLOOR && gs.fg_map[py][px] == FG_OBJ_NONE) {
            bool reachable = false;
            for (int b = 0; b < placed; b++) {
                const int dx[] = {0, 0, -1, 1};
                const int dy[] = {-1, 1, 0, 0};
                for (int d = 0; d < 4; d++) {
                    int adjx = boxes[b].x + dx[d];
                    int adjy = boxes[b].y + dy[d];
                    if (adjx >= 1 && adjx < MAP_W - 1 && adjy >= 1 && adjy < MAP_H - 1) {
                        if (flood_check(gs.bg_map, px, py, adjx, adjy)) {
                            reachable = true;
                            break;
                        }
                    }
                }
                if (reachable) break;
            }
            if (reachable) {
                player_placed = true;
                break;
            }
        }
    }

    if (!player_placed) return false;

    // Step 4: Do reverse pushes to separate boxes from goals
    int reverse_moves = rng_range(5 + difficulty * 5, 15 + difficulty * 10);

    for (int m = 0; m < reverse_moves; m++) {
        int bi = rng_range(0, placed - 1);
        int bx = boxes[bi].x;
        int by = boxes[bi].y;

        const int dx[] = {0, 0, -1, 1};
        const int dy[] = {-1, 1, 0, 0};
        int d = rng_range(0, 3);

        int new_bx = bx + dx[d];
        int new_by = by + dy[d];
        int player_bx = bx - dx[d];
        int player_by = by - dy[d];

        if (new_bx < 1 || new_bx >= MAP_W - 1 || new_by < 1 || new_by >= MAP_H - 1) continue;
        if (player_bx < 1 || player_bx >= MAP_W - 1 || player_by < 1 || player_by >= MAP_H - 1) continue;

        unsigned char dest_bg = gs.bg_map[new_by][new_bx];
        unsigned char dest_fg = gs.fg_map[new_by][new_bx];
        unsigned char player_dest_bg = gs.bg_map[player_by][player_bx];
        unsigned char player_dest_fg = gs.fg_map[player_by][player_bx];

        if (dest_bg != BG_TILE_FLOOR && dest_bg != BG_TILE_SWITCH) continue;
        if (dest_fg != FG_OBJ_NONE) continue;
        if (player_dest_bg == BG_TILE_WALL || player_dest_fg != FG_OBJ_NONE) continue;

        // Perform reverse push
        gs.fg_map[by][bx] = FG_OBJ_NONE;
        gs.fg_map[new_by][new_bx] = FG_OBJ_BARREL;
        boxes[bi] = {new_bx, new_by};
    }

    // Step 5: Verify at least one box is NOT on goal
    bool has_unsolved = false;
    for (int b = 0; b < placed; b++) {
        if (gs.bg_map[boxes[b].y][boxes[b].x] != BG_TILE_SWITCH) {
            has_unsolved = true;
            break;
        }
    }
    if (!has_unsolved) return false;

    // Step 6: Place player and finalize
    gs.fg_map[py][px] = FG_OBJ_PLAYER;
    gs.player_x = px;
    gs.player_y = py;
    gs.shady_x = -1;
    gs.shady_y = -1;
    gs.moves = 0;
    gs.cleared = false;
    gs.failed = false;
    gs.current_level = -1;
    gs.dropped_barrels = 0;

    return true;
}
