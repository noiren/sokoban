#ifndef TITLE_STATE_H
#define TITLE_STATE_H

#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "state/state.h"
#include "../gfx/ui_manager.h"
#include "../gfx/fade_effect.h"

// タイトル画面の内部フェーズ
enum class TitlePhase {
    EPID_LOGO_DISP,       // EPIDロゴ表示
    DOUJIN_NOTICE_DISP,   // 同人注意書き表示
    AUTOSAVE_WARN_DISP,   // オートセーブ警告表示
    TITLE_DISP,           // タイトル表示
};

class TitleState : public State {
public:
    TitleState(bn::sprite_text_generator& text_gen);

    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

private:
    void go_to_phase(TitlePhase next);

    void update_epid_logo(StateManager& manager);
    void update_doujin_notice(StateManager& manager);
    void update_autosave_warn(StateManager& manager);
    void update_title(StateManager& manager);

    bn::sprite_text_generator& text_gen_;
    TitlePhase   phase_;
    PhaseStep    step_;
    int          frame_counter_;
    int          blink_counter_;
    FadeEffect   fade_;
    UIManager    ui_manager_;
};

#endif // TITLE_STATE_H
