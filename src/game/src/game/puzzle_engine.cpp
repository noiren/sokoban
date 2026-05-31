#include "game/puzzle_engine.h"
#include "bn_math.h"

// levels.h は puzzle_engine.cpp と同じ game/ ディレクトリにあるため、ファイル名のみで参照可
#include "game/levels.h"

// =============================================================================
// 公開API
// =============================================================================

void PuzzleEngine::load_level(int level_id) {
    if (level_id < 0) level_id = 0;
    if (level_id >= NUM_LEVELS) level_id = NUM_LEVELS - 1;

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            data_.bg_map[y][x] = BgTile::EMPTY;
            data_.fg_map[y][x] = FgObj::NONE;
        }
    }

    int offset_x = (MAP_W - 15) / 2;
    int offset_y = (MAP_H - 10) / 2;

    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 15; x++) {
            data_.bg_map[y + offset_y][x + offset_x] = static_cast<BgTile>(level_bg_data[level_id][y][x]);
            data_.fg_map[y + offset_y][x + offset_x] = static_cast<FgObj>(level_fg_data[level_id][y][x]);
        }
    }

    data_.player_x        = level_player_x[level_id] + offset_x;
    data_.player_y        = level_player_y[level_id] + offset_y;
    data_.shady_x         = level_shady_x[level_id] == -1 ? -1 : level_shady_x[level_id] + offset_x;
    data_.shady_y         = level_shady_y[level_id] == -1 ? -1 : level_shady_y[level_id] + offset_y;
    data_.moves           = 0;
    data_.dropped_barrels = 0;
    events_.clear();
}

PuzzleEngine::Result PuzzleEngine::try_move(int dx, int dy) {
    events_.clear();

    int nx = data_.player_x + dx;
    int ny = data_.player_y + dy;

    if (!in_bounds(nx, ny)) return Result::CONTINUE;

    bool sw = are_switches_pressed();

    // 壁なら即座に何もしない
    BgTile dest_bg = data_.bg_map[ny][nx];
    if (dest_bg == BgTile::WALL) return Result::CONTINUE;
    // ゴール/EXITは鍵がかかっているとき通行不可
    if ((dest_bg == BgTile::GOAL || dest_bg == BgTile::EXIT) && !sw) return Result::CONTINUE;

    // 移動先に樽があれば押す
    FgObj dest_fg = data_.fg_map[ny][nx];
    if (dest_fg == FgObj::BARREL) {
        if (!try_push_barrel(nx, ny, dx, dy)) return Result::CONTINUE;
    }
    // シェイディがいれば移動不可（ボスは捕まえる側なので通り抜けはさせない）
    if (dest_fg == FgObj::SHADY) return Result::CONTINUE;

    // プレイヤーを1マス動かす
    data_.fg_map[data_.player_y][data_.player_x] = FgObj::NONE;
    push_event({EventType::MOVE_FG,
                (uint8_t)data_.player_x, (uint8_t)data_.player_y,
                (int8_t)dx, (int8_t)dy,
                BgTile::FLOOR, 0});
    data_.player_x = nx;
    data_.player_y = ny;
    data_.fg_map[ny][nx] = FgObj::PLAYER;
    data_.moves++;

    // 足元タイルの on_enter を呼ぶ
    BgTile stepped = data_.bg_map[ny][nx];
    const TileHandler& h = TILE_HANDLERS[(int)stepped];
    if (h.on_enter) {
        h.on_enter(*this, nx, ny, FgObj::PLAYER);
    }

    // 位置変えスイッチを踏んだら矢印を全回転
    if (stepped == BgTile::TOGGLE_SWITCH) {
        rotate_arrows();
    }

    // 連鎖移動（氷・矢印・ワープ）
    process_chain_move(data_.player_x, data_.player_y, dx, dy, FgObj::PLAYER);

    // 穴に落ちたか
    if (data_.bg_map[data_.player_y][data_.player_x] == BgTile::HOLE) {
        return Result::FAILED;
    }

    // ボスステージではシェイディが動く
    if (data_.shady_x != -1 && data_.shady_y != -1) {
        move_shady();
    }

    // クリア判定
    if (check_cleared()) {
        return Result::CLEARED;
    }

    return Result::CONTINUE;
}

void PuzzleEngine::change_bg(int x, int y, BgTile new_tile) {
    data_.bg_map[y][x] = new_tile;
    push_event({EventType::CHANGE_BG,
                (uint8_t)x, (uint8_t)y,
                0, 0,
                new_tile, 0});
}

void PuzzleEngine::push_event(const PuzzleEvent& e) {
    if (!events_.full()) {
        events_.push_back(e);
    }
}

// =============================================================================
// 内部ヘルパー
// =============================================================================

bool PuzzleEngine::in_bounds(int x, int y) {
    return x >= 0 && x < MAP_W && y >= 0 && y < MAP_H;
}

bool PuzzleEngine::are_switches_pressed() const {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (data_.bg_map[y][x] == BgTile::SWITCH &&
                data_.fg_map[y][x] != FgObj::BARREL) {
                return false;
            }
        }
    }
    return true;
}

bool PuzzleEngine::is_passable_for_mover(int x, int y) const {
    if (!in_bounds(x, y)) return false;
    BgTile bg = data_.bg_map[y][x];
    if (bg == BgTile::WALL) return false;
    if ((bg == BgTile::GOAL || bg == BgTile::EXIT) && !are_switches_pressed()) return false;
    if (data_.fg_map[y][x] != FgObj::NONE) return false;
    return true;
}

bool PuzzleEngine::is_passable_for_barrel(int x, int y) const {
    if (!in_bounds(x, y)) return false;
    BgTile bg = data_.bg_map[y][x];
    if (bg == BgTile::WALL) return false;
    if ((bg == BgTile::GOAL || bg == BgTile::EXIT) && !are_switches_pressed()) return false;
    if (data_.fg_map[y][x] != FgObj::NONE) return false;
    // HOLE は樽で埋められるので passable
    return true;
}

void PuzzleEngine::find_warp_dest(BgTile warp_tile, int src_x, int src_y,
                                   int& dst_x, int& dst_y) const {
    BgTile target = BgTile::EMPTY;
    if      (warp_tile == BgTile::WARP_RED_A)  target = BgTile::WARP_RED_B;
    else if (warp_tile == BgTile::WARP_RED_B)  target = BgTile::WARP_RED_A;
    else if (warp_tile == BgTile::WARP_BLUE_A) target = BgTile::WARP_BLUE_B;
    else if (warp_tile == BgTile::WARP_BLUE_B) target = BgTile::WARP_BLUE_A;
    else { dst_x = src_x; dst_y = src_y; return; }

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if ((x != src_x || y != src_y) && data_.bg_map[y][x] == target) {
                dst_x = x; dst_y = y;
                return;
            }
        }
    }
    dst_x = src_x; dst_y = src_y;
}

void PuzzleEngine::rotate_arrows() {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            BgTile bg = data_.bg_map[y][x];
            BgTile next = bg;
            if      (bg == BgTile::ARROW_UP)    next = BgTile::ARROW_RIGHT;
            else if (bg == BgTile::ARROW_RIGHT)  next = BgTile::ARROW_DOWN;
            else if (bg == BgTile::ARROW_DOWN)   next = BgTile::ARROW_LEFT;
            else if (bg == BgTile::ARROW_LEFT)   next = BgTile::ARROW_UP;
            if (next != bg) {
                data_.bg_map[y][x] = next;
                // BGの変化をイベントとして通知
                push_event({EventType::CHANGE_BG,
                            (uint8_t)x, (uint8_t)y,
                            0, 0, next, 0});
            }
        }
    }
}

bool PuzzleEngine::try_push_barrel(int barrel_x, int barrel_y, int dx, int dy) {
    int bx = barrel_x + dx;
    int by = barrel_y + dy;

    if (!is_passable_for_barrel(bx, by)) return false;

    // 樽を元の位置から取り除く
    data_.fg_map[barrel_y][barrel_x] = FgObj::NONE;
    push_event({EventType::MOVE_FG,
                (uint8_t)barrel_x, (uint8_t)barrel_y,
                (int8_t)dx, (int8_t)dy,
                BgTile::FLOOR, 0});

    // 穴に落ちる場合は穴を埋めて樽消滅
    if (data_.bg_map[by][bx] == BgTile::HOLE) {
        change_bg(bx, by, BgTile::FLOOR);
        data_.dropped_barrels++;
        return true;
    }

    // 通常移動
    data_.fg_map[by][bx] = FgObj::BARREL;

    // 樽にも連鎖移動を適用（氷・矢印）
    process_chain_move(bx, by, dx, dy, FgObj::BARREL);
    return true;
}

void PuzzleEngine::process_chain_move(int& x, int& y, int dx, int dy, FgObj obj) {
    while (true) {
        BgTile current = data_.bg_map[y][x];
        const TileHandler& h = TILE_HANDLERS[(int)current];

        // on_enter は1マス移動のたびに呼ぶ（連鎖中も床は崩れる）
        // ただし最初の1歩は try_move / try_push_barrel 側で呼んでいるため、
        // ここでは「連鎖で新しいマスに到達した場合のみ」呼ぶ。
        // → ループの末尾で呼ぶ設計にするため、ここではスキップ。

        // 次の方向を計算
        int next_dx = dx, next_dy = dy;
        if (!h.get_next_move || !h.get_next_move(*this, x, y, next_dx, next_dy)) {
            break; // このタイルは連鎖しない（通常床など）
        }

        // ワープタイルの判定
        bool is_warp = (current == BgTile::WARP_RED_A  || current == BgTile::WARP_RED_B ||
                        current == BgTile::WARP_BLUE_A || current == BgTile::WARP_BLUE_B);
        if (is_warp) {
            int wx = x, wy = y;
            find_warp_dest(current, x, y, wx, wy);
            if ((wx != x || wy != y) && data_.fg_map[wy][wx] == FgObj::NONE) {
                data_.fg_map[y][x] = FgObj::NONE;
                // ワープは絶対座標移動としてイベントを積む（dx/dyをオフセットで表現）
                push_event({EventType::MOVE_FG,
                            (uint8_t)x, (uint8_t)y,
                            (int8_t)(wx - x), (int8_t)(wy - y),
                            BgTile::FLOOR, 0});
                x = wx; y = wy;
                data_.fg_map[y][x] = obj;
                // ワープ後のマスで on_enter を呼ぶ
                const TileHandler& wh = TILE_HANDLERS[(int)data_.bg_map[y][x]];
                if (wh.on_enter) wh.on_enter(*this, x, y, obj);
                continue;
            }
            break; // ワープ先が埋まっている場合は停止
        }

        int next_x = x + next_dx;
        int next_y = y + next_dy;

        if (!in_bounds(next_x, next_y)) break;

        // 移動先が通れるか（オブジェクトの種別で判断）
        bool passable = (obj == FgObj::BARREL)
                        ? is_passable_for_barrel(next_x, next_y)
                        : is_passable_for_mover(next_x, next_y);
        if (!passable) break;

        // 実際に移動
        data_.fg_map[y][x] = FgObj::NONE;
        push_event({EventType::MOVE_FG,
                    (uint8_t)x, (uint8_t)y,
                    (int8_t)next_dx, (int8_t)next_dy,
                    BgTile::FLOOR, 0});
        x = next_x; y = next_y;
        data_.fg_map[y][x] = obj;
        dx = next_dx; dy = next_dy;

        // 新しいマスで on_enter を呼ぶ
        const TileHandler& nh = TILE_HANDLERS[(int)data_.bg_map[y][x]];
        if (nh.on_enter) {
            nh.on_enter(*this, x, y, obj);
        }
    }
}

bool PuzzleEngine::check_cleared() const {
    // ボスステージ：プレイヤーがシェイディに隣接したらクリア
    if (data_.shady_x != -1 && data_.shady_y != -1) {
        int adx = bn::abs(data_.player_x - data_.shady_x);
        int ady = bn::abs(data_.player_y - data_.shady_y);
        return (adx + ady <= 1);
    }

    // 通常ステージ：ゴール（またはEXIT）に立ち かつ 全スイッチ押下
    if (data_.bg_map[data_.player_y][data_.player_x] == BgTile::GOAL ||
        data_.bg_map[data_.player_y][data_.player_x] == BgTile::EXIT) {
        return are_switches_pressed();
    }

    return false;
}

void PuzzleEngine::move_shady() {
    int best_x = data_.shady_x;
    int best_y = data_.shady_y;
    int max_dist = (data_.player_x - data_.shady_x) * (data_.player_x - data_.shady_x)
                 + (data_.player_y - data_.shady_y) * (data_.player_y - data_.shady_y);

    const int dxs[] = { 0, 0, -1, 1 };
    const int dys[] = { -1, 1, 0, 0 };

    for (int i = 0; i < 4; i++) {
        int nx = data_.shady_x + dxs[i];
        int ny = data_.shady_y + dys[i];
        if (!is_passable_for_mover(nx, ny)) continue;

        int dist = (data_.player_x - nx) * (data_.player_x - nx)
                 + (data_.player_y - ny) * (data_.player_y - ny);
        if (dist > max_dist) {
            max_dist = dist;
            best_x = nx;
            best_y = ny;
        }
    }

    if (best_x == data_.shady_x && best_y == data_.shady_y) return;

    int move_dx = best_x - data_.shady_x;
    int move_dy = best_y - data_.shady_y;

    data_.fg_map[data_.shady_y][data_.shady_x] = FgObj::NONE;
    push_event({EventType::MOVE_FG,
                (uint8_t)data_.shady_x, (uint8_t)data_.shady_y,
                (int8_t)move_dx, (int8_t)move_dy,
                BgTile::FLOOR, 0});
    data_.shady_x = best_x;
    data_.shady_y = best_y;
    data_.fg_map[best_y][best_x] = FgObj::SHADY;

    // シェイディにも連鎖移動を適用
    process_chain_move(data_.shady_x, data_.shady_y, move_dx, move_dy, FgObj::SHADY);
}
