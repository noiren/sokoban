#ifndef TITLE_STATE_H
#define TITLE_STATE_H

#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "state/state.h"
#include "gfx/ui_manager.h"
#include "gfx/fade_effect.h"

// タイトル画面の内部フェーズ
enum class TitlePhase {
    EPID_LOGO_DISP,       // EPIDロゴ表示
    DOUJIN_NOTICE_DISP,   // 同人注意書き表示
    AUTOSAVE_WARN_DISP,   // オートセーブ警告表示
    TITLE_DISP,           // タイトル表示
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
    void go_to_phase(TitlePhase next);

    void update_epid_logo(StateManager& sm, SharedContext& ctx);
    void update_doujin_notice(StateManager& sm, SharedContext& ctx);
    void update_autosave_warn(StateManager& sm, SharedContext& ctx);
    void update_title(StateManager& sm, SharedContext& ctx);

    bn::optional<UIManager> ui_manager_;
    TitlePhase   phase_;
    PhaseStep    step_;
    int          frame_counter_;
    int          blink_counter_;
    FadeEffect   fade_;
};

#endif // TITLE_STATE_H
