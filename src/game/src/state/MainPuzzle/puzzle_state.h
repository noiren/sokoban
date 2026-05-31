#ifndef PUZZLE_STATE_H
#define PUZZLE_STATE_H

#include "state/state.h"
#include "game/puzzle_engine.h"

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_array.h"
#include "bn_optional.h"
#include "bn_string.h"
#include "bn_camera_ptr.h"
#include "ui/Core/Manager/ui_manager.h"

// パズル画面内のフェーズ
enum class PuzzlePhase {
    PLAYING,    // 入力待ち・通常プレイ中
    ANIMATING,  // イベントキューを消費してアニメーション中
    FAILED,     // 落下ミス（リトライ待ち）
    CLEARED,    // クリア（次ステージ遷移待ち）
    COUNT
};

class PuzzleState : public State {
public:
    PuzzleState();

    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    // --- フェーズテーブル駆動 ---
    using EnterExitFunc = void (PuzzleState::*)();
    using UpdateFunc    = void (PuzzleState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[(int)PuzzlePhase::COUNT];

    void change_phase(PuzzlePhase next);

    void enter_playing();
    void update_playing(StateManager& sm, SharedContext& ctx);
    void exit_playing();

    void enter_animating();
    void update_animating(StateManager& sm, SharedContext& ctx);
    void exit_animating();

    void enter_failed();
    void update_failed(StateManager& sm, SharedContext& ctx);
    void exit_failed();

    void enter_cleared();
    void update_cleared(StateManager& sm, SharedContext& ctx);
    void exit_cleared();

    // --- 内部ヘルパー ---
    void level_init();
    void redraw_map();
    void update_hud();
    void update_camera();

    // --- メンバ変数 ---
    bn::optional<UIManager> ui_manager_;
    PuzzleEngine engine_;               // EWRAMに静的に確保される（newなし）

    PuzzlePhase phase_;
    PuzzleEngine::Result last_result_;  // try_moveの結果をANIMATINGフェーズ後まで保持

    int current_level_;
    int last_drawn_moves_;              // HUDの差分更新用

    // アニメーション制御
    int current_event_index_;
    int anim_frame_;

    // BGマップ描画用
    bn::optional<bn::regular_bg_ptr>     bg_;
    bn::optional<bn::regular_bg_map_ptr> bg_map_;
    bn::optional<bn::camera_ptr>         camera_;
    alignas(4) bn::array<bn::regular_bg_map_cell, 32 * 32> map_cells_;
};

#endif // PUZZLE_STATE_H
