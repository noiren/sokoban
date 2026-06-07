#ifndef DEBUG_STATE_H
#define DEBUG_STATE_H

#include "state/state.h"

#include "generated/audio_ids.h"
#include "generated/sprite_anim_data.h"
#include "animation/sprite_anim_manager.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_fixed.h"
#include "bn_vector.h"
#include "bn_optional.h"
#include "ui/Core/Manager/ui_manager.h"
#include "save/save_data.h"
#include "save/user_data_puzzle.h"
#include "game/levels.h"

enum class DebugScreen {
    Root,
    UiList,         // 全レイアウト(ui_data_*)の表示チェック
    PortraitSolo,   // 立ち絵スプライト単体プレビュー(拡大)
    BgmList,
    BgmTest,
    SeList,
    SeTest,
    EffectTest,
    EventTest,      // イベント再生デバッグ
    AnimTest,       // スプライトアニメーションデバッグ
    StageList,      // ステージ選んでプレイデバッグ
    StillEventTest, // スチルイベント再生デバッグ
    ProgressData,   // 知識値（フラグ・パズルクリア）デバッグ
};

class DebugState : public State {
public:
    DebugState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}
    void resume(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}

private:
    void redraw(SharedContext& ctx);

    void update_root(StateManager& sm, SharedContext& ctx);
    void draw_root(SharedContext& ctx);

    void update_ui_list(StateManager& sm, SharedContext& ctx);
    void draw_ui_list(SharedContext& ctx);
    void _ui_debug_reload(SharedContext& ctx);
    void _ui_debug_apply_event_portraits();

    void update_portrait_solo(StateManager& sm, SharedContext& ctx);
    void draw_portrait_solo(SharedContext& ctx);
    void _portrait_solo_refresh();

    void update_bgm_list(StateManager& sm, SharedContext& ctx);
    void draw_bgm_list(SharedContext& ctx);

    void update_bgm_test(StateManager& sm, SharedContext& ctx);
    void draw_bgm_test(SharedContext& ctx);

    void update_se_list(StateManager& sm, SharedContext& ctx);
    void draw_se_list(SharedContext& ctx);

    void update_se_test(StateManager& sm, SharedContext& ctx);
    void draw_se_test(SharedContext& ctx);

    void update_effect_test(StateManager& sm, SharedContext& ctx);
    void draw_effect_test(SharedContext& ctx);

    void update_event_test(StateManager& sm, SharedContext& ctx);
    void draw_event_test(SharedContext& ctx);

    void update_anim_test(StateManager& sm, SharedContext& ctx);
    void draw_anim_test(SharedContext& ctx);

    void update_still_event_test(StateManager& sm, SharedContext& ctx);
    void draw_still_event_test(SharedContext& ctx);

    void update_stage_list(StateManager& sm, SharedContext& ctx);
    void draw_stage_list(SharedContext& ctx);

    void update_progress_data(StateManager& sm, SharedContext& ctx);
    void draw_progress_data(SharedContext& ctx);

    [[nodiscard]] bool _bgm_test_track_is_playing() const;

    DebugScreen screen_;
    int cursor_;
    int ui_debug_cursor_ = 0;   // UI DEBUG: 表示中レイアウト番号
    int ui_portrait_idx_ = 0;   // UI DEBUG: event レイアウト時の char_left 用 portrait 番号
    bn::optional<UIManager> ui_manager_;
    bn::optional<bn::sprite_ptr> portrait_solo_sprite_; // PortraitSolo 用(単体拡大)

    int event_cursor_;    // イベントテスト用カーソル
    int progress_cursor_; // 知識値デバッグ用カーソル
    int progress_sub_;    // 0=トップ 1=イベント個別 2=パズル個別

    BgmId test_bgm_id_;
    SeId test_se_id_;
    bool last_drawn_bgm_playing_;
    AnimHandle anim_test_handle_ = INVALID_ANIM_HANDLE;

    bn::vector<bn::sprite_ptr, 128> sprites_;

    bool prev_sound_bgm_enabled_;
    bool prev_sound_se_enabled_;

    bn::fixed prev_sprite_intensity_;
    bool had_saved_sprite_intensity_;

    PhaseStep step_;
};

#endif
