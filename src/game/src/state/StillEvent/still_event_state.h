#ifndef STILL_EVENT_STATE_H
#define STILL_EVENT_STATE_H

#include "state/state.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"
#include "ui/Core/Effects/fade_effect.h"
#include "generated/generated_fix_data.h"

// ============================================================
// StillEventPhase
// ============================================================
enum class StillEventPhase {
    FADE_IN,       // フェードイン
    SHOWING,       // スチル表示中・テキスト送り
    FADE_OUT,      // フェードアウト
    COUNT
};

// ============================================================
// StillEventState
//
// 演出：
//   上3/4 = スチル画像（regular_bg）
//   下1/4 = テキストのみ（ダイアログボックスなし）
//
// SharedContextからの入力：
//   ctx.target_event_id = スチルイベントのID（FdEventEntryとして検索）
//
// 完了時の出力：
//   ctx.story_step_completed = true
//   sm.pop_state()
// ============================================================
class StillEventState : public State {
public:
    static constexpr int TYPING_SPEED_NORMAL = 2;
    static constexpr int TYPING_SPEED_FAST   = 1;
    static constexpr int FADE_DURATION       = 16; // フェード時間（フレーム）

    StillEventState();

    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void change_phase(StillEventPhase next);

    void enter_fade_in();
    void update_fade_in(StateManager& sm, SharedContext& ctx);
    void exit_fade_in();

    void enter_showing();
    void update_showing(StateManager& sm, SharedContext& ctx);
    void exit_showing();

    void enter_fade_out();
    void update_fade_out(StateManager& sm, SharedContext& ctx);
    void exit_fade_out();

    // テキスト描画
    void apply_line(int line_index, SharedContext& ctx);
    void advance_typewriter(int chars_per_frame);
    void refresh_display_text(SharedContext& ctx);

    using EnterExitFunc = void (StillEventState::*)();
    using UpdateFunc    = void (StillEventState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];

    // イベントデータ
    const FdEventEntry* event_entry_;
    int pc_;                          // 現在の行インデックス
    StillEventPhase phase_;
    PhaseStep step_;

    // タイプライター
    bn::string_view typewrite_text_;
    int displayed_chars_;
    int text_timer_;
    bool skip_requested_;

    // テキスト表示用スプライト（sprite_text_generator 結果）
    bn::vector<bn::sprite_ptr, 64> text_sprites_;

    // フェード
    FadeEffect fade_;

    // スチル背景（TODO: 実際のスチル画像リソースに置き換える）
    bn::optional<bn::regular_bg_ptr> still_bg_;
};

#endif // STILL_EVENT_STATE_H
