#ifndef TITLE_STATE_H
#define TITLE_STATE_H

#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "state/state.h"
#include "ui/Core/Manager/ui_manager.h"
#include "ui/Core/Effects/fade_effect.h"

// タイトル画面の内部フェーズ
enum class TitlePhase {
    EPID_LOGO_DISP,       // EPIDロゴ表示
    DOUJIN_NOTICE_DISP,   // 同人注意書き表示
    AUTOSAVE_WARN_DISP,   // オートセーブ警告表示
    TITLE_DISP,           // タイトル表示
    COUNT                 // フェーズの総数（テーブル配列のサイズ用）
};


class TitleState : public State {
public:
    TitleState();

    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    // 汎用フェーズ遷移関数
    void change_phase(TitlePhase next);

    // ==========================================
    // 各フェーズの処理関数群
    // ==========================================
    // EPIDロゴ
    void enter_epid_logo();
    void update_epid_logo(StateManager& sm, SharedContext& ctx);
    void exit_epid_logo();

    // 同人注意書き
    void enter_doujin_notice();
    void update_doujin_notice(StateManager& sm, SharedContext& ctx);
    void exit_doujin_notice();

    // オートセーブ警告
    void enter_autosave_warn();
    void update_autosave_warn(StateManager& sm, SharedContext& ctx);
    void exit_autosave_warn();

    // タイトル
    void enter_title();
    void update_title(StateManager& sm, SharedContext& ctx);
    void exit_title();

    // ==========================================
    // 関数ポインタ・テーブル定義
    // ==========================================
    using EnterExitFunc = void (TitleState::*)();
    using UpdateFunc = void (TitleState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];

    // ==========================================
    // メンバ変数
    // ==========================================
    bn::optional<UIManager> ui_manager_;
    TitlePhase   phase_;
    PhaseStep    step_;
    int          frame_counter_;
    int          blink_counter_;
    FadeEffect   fade_;
};

#endif // TITLE_STATE_H