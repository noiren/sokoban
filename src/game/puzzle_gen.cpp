#include "puzzle_gen.h"

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
    return gs.map[y][x] == TILE_FLOOR;
}

// Simple flood-fill reachability check
static bool flood_check(const unsigned char map[MAP_H][MAP_W], int sx, int sy, int tx, int ty) {
    bool visited[MAP_H][MAP_W] = {};
    // Simple BFS with stack (limited size for GBA)
    struct Pos { int x, y; };
    Pos stack[MAP_W * MAP_H];
    int top = 0;

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
                !visited[ny][nx] && map[ny][nx] != TILE_WALL) {
                visited[ny][nx] = true;
                stack[top++] = {nx, ny};
            }
        }
    }
    return false;
}

// Generate a puzzle by "reverse solving" — place player, boxes on goals,
// then simulate random reverse pushes to create a solvable puzzle
bool puzzle_generate(GameState& gs, int difficulty, int seed) {
    rng_seed(seed);

    int num_boxes = 1 + difficulty;
    if (num_boxes > 3) num_boxes = 3;

    // Step 1: Create room with random walls
    // Border walls
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (x == 0 || x == MAP_W - 1 || y == 0 || y == MAP_H - 1) {
                gs.map[y][x] = TILE_WALL;
            } else {
                gs.map[y][x] = TILE_FLOOR;
            }
        }
    }

    // Add some random interior walls (20-35%)
    int wall_count = rng_range(8, 18);
    for (int i = 0; i < wall_count; i++) {
        int wx = rng_range(2, MAP_W - 3);
        int wy = rng_range(2, MAP_H - 3);
        gs.map[wy][wx] = TILE_WALL;
    }

    // Step 2: Place goals and boxes (boxes start ON goals for reverse solving)
    struct Pos { int x, y; };
    Pos goals[3];
    Pos boxes[3];
    int placed = 0;

    for (int attempt = 0; attempt < 100 && placed < num_boxes; attempt++) {
        int gx = rng_range(2, MAP_W - 3);
        int gy = rng_range(2, MAP_H - 3);
        if (gs.map[gy][gx] == TILE_FLOOR) {
            // Check not adjacent to another goal
            bool ok = true;
            for (int j = 0; j < placed; j++) {
                int dx = gx - goals[j].x;
                int dy = gy - goals[j].y;
                if (dx * dx + dy * dy < 4) { ok = false; break; }
            }
            if (ok) {
                goals[placed] = {gx, gy};
                boxes[placed] = {gx, gy};  // Start on goal
                gs.map[gy][gx] = TILE_BOX_ON_GOAL;
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
        if (gs.map[py][px] == TILE_FLOOR) {
            // Check reachability to at least one box
            bool reachable = false;
            for (int b = 0; b < placed; b++) {
                // Check adjacent to box
                const int dx[] = {0, 0, -1, 1};
                const int dy[] = {-1, 1, 0, 0};
                for (int d = 0; d < 4; d++) {
                    int adjx = boxes[b].x + dx[d];
                    int adjy = boxes[b].y + dy[d];
                    if (adjx >= 1 && adjx < MAP_W - 1 && adjy >= 1 && adjy < MAP_H - 1) {
                        if (flood_check(gs.map, px, py, adjx, adjy)) {
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
    // Reverse push = player stands opposite side, pushes box away
    int reverse_moves = rng_range(5 + difficulty * 5, 15 + difficulty * 10);

    for (int m = 0; m < reverse_moves; m++) {
        // Pick a random box
        int bi = rng_range(0, placed - 1);
        int bx = boxes[bi].x;
        int by = boxes[bi].y;

        // Pick a random direction
        const int dx[] = {0, 0, -1, 1};
        const int dy[] = {-1, 1, 0, 0};
        int d = rng_range(0, 3);

        // Reverse push: box moves in direction d, player was behind box
        int new_bx = bx + dx[d];
        int new_by = by + dy[d];
        int player_bx = bx - dx[d];  // Where player would stand
        int player_by = by - dy[d];

        // Validate
        if (new_bx < 1 || new_bx >= MAP_W - 1 || new_by < 1 || new_by >= MAP_H - 1) continue;
        if (player_bx < 1 || player_bx >= MAP_W - 1 || player_by < 1 || player_by >= MAP_H - 1) continue;

        unsigned char dest = gs.map[new_by][new_bx];
        unsigned char player_dest = gs.map[player_by][player_bx];

        if (dest != TILE_FLOOR && dest != TILE_GOAL) continue;
        if (player_dest == TILE_WALL || player_dest == TILE_BOX ||
            player_dest == TILE_BOX_ON_GOAL) continue;

        // Execute reverse push
        // Remove box from current position
        bool was_on_goal = (gs.map[by][bx] == TILE_BOX_ON_GOAL);
        gs.map[by][bx] = was_on_goal ? TILE_GOAL : TILE_FLOOR;

        // Place box at new position
        gs.map[new_by][new_bx] = (dest == TILE_GOAL) ? TILE_BOX_ON_GOAL : TILE_BOX;
        boxes[bi] = {new_bx, new_by};
    }

    // Step 5: Verify at least one box is NOT on goal (otherwise puzzle is already solved)
    bool has_unsolved = false;
    for (int b = 0; b < placed; b++) {
        if (gs.map[boxes[b].y][boxes[b].x] == TILE_BOX) {
            has_unsolved = true;
            break;
        }
    }
    if (!has_unsolved) return false;

    // Step 6: Place player and finalize
    gs.map[py][px] = TILE_PLAYER;
    gs.player_x = px;
    gs.player_y = py;
    gs.moves = 0;
    gs.cleared = false;
    gs.current_level = -1;

    return true;
}
