#ifndef PUZZLE_ENGINE_H
#define PUZZLE_ENGINE_H

#include "puzzle_data.h"
#include "tile_handler.h"
#include "bn_vector.h"

struct GameState; // sokoban.h（手動生成盤面のロード用）

// --- パズルのゲームロジック本体 ---
// ・入力を受けて盤面を1手進める
// ・ギミックの連鎖（氷・矢印・ワープ・ボロボロ床・シェイディAI）を処理する
// ・描画・演出側に渡す PuzzleEvent をキューに積む
// ・UIや StateManager には一切触れない（純粋なロジック層）
class PuzzleEngine {
public:
    // try_move の戻り値
    enum class Result {
        CONTINUE, // 何も変わらず継続
        FAILED,   // プレイヤーが穴に落ちた
        CLEARED   // クリア条件を満たした
    };

    // レベルデータをロードして盤面を初期化する
    void load_level(int level_id);

    // puzzle_generate 等で構築した GameState をロードする（エンドレス用）
    void load_from_game_state(const GameState& gs);

    // 入力を受けて1手進める。結果を返す。
    Result try_move(int dx, int dy);

    // 描画用イベントキューへのアクセス
    const bn::vector<PuzzleEvent, 64>& events() const { return events_; }
    void clear_events() { events_.clear(); }

    // 盤面データへのアクセス（描画・HUD更新に使う）
    const PuzzleData& data() const { return data_; }

    // 1手戻る（Undo）。成功したら true を返す
    bool try_undo();

    // タイルハンドラから呼ばれるAPI（内部からも呼ばれる）
    void change_bg(int x, int y, BgTile new_tile);
    void push_event(const PuzzleEvent& e);

private:
    static constexpr int HISTORY_SIZE = 30;

    PuzzleData data_;
    bn::vector<PuzzleEvent, 64> events_;

    PuzzleData history_[HISTORY_SIZE];
    int history_count_ = 0;
    int history_head_ = 0;

    void record_history();

    // --- 内部ヘルパー ---

    // 全スイッチが樽で押されているか
    bool are_switches_pressed() const;

    // 通常移動（プレイヤー・シェイディ）が可能か
    bool is_passable_for_mover(int x, int y) const;

    // 樽の移動先として有効か（HOLEはOK、樽・プレイヤーはNG）
    bool is_passable_for_barrel(int x, int y) const;

    // 範囲チェック
    static bool in_bounds(int x, int y);

    // ワープ先の座標を探す
    void find_warp_dest(BgTile warp_tile, int src_x, int src_y, int& dst_x, int& dst_y) const;

    // 全矢印パネルを時計回りに90度回転
    void rotate_arrows();

    // 樽を押す処理（移動・HOLE埋め・連鎖移動を含む）
    // 移動できなければ false を返す
    bool try_push_barrel(int barrel_x, int barrel_y, int dx, int dy);

    // 連鎖移動（氷・矢印・ワープ）。停止するまでループ。
    // x, y を更新し、移動イベントを積む。
    void process_chain_move(int& x, int& y, int dx, int dy, FgObj obj);

    // クリア条件チェック
    bool check_cleared() const;

    // シェイディのAI移動（ボスステージ）
    void move_shady();
};

#endif // PUZZLE_ENGINE_H
