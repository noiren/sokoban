#ifndef TITLE_STATE_H
#define TITLE_STATE_H

#include "state/state.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "../gfx/ui_manager.h"

// 自動生成UIデータヘッダ
#include "ui_data_logo.h"
#include "ui_data_attention.h"
#include "ui_data_autosave_attension.h"
#include "ui_data_title.h"

// 起動シーケンスのシーン単位フェーズ
// EPID_LOGO_DISP     : "produced by EPID GAMES" ロゴ表示
// DOUJIN_NOTICE_DISP : "この作品は同人作品です" 注意書き
// AUTOSAVE_WARN_DISP : オートセーブ警告メッセージ
// TITLE_DISP         : タイトル画面 (PRESS START 点滅) → SaveSelectへ
enum class TitlePhase {
    EPID_LOGO_DISP,
    DOUJIN_NOTICE_DISP,
    AUTOSAVE_WARN_DISP,
    TITLE_DISP,
};


class TitleState : public State {
public:
    explicit TitleState(bn::sprite_text_generator& text_gen);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

private:
    // フェードヘルパー
    bool fade_in(int duration);
    bool fade_out(int duration);

    // フェーズ遷移（画面ロード・カウンタリセットを一括管理）
    void go_to_phase(TitlePhase next);

    // 各フェーズのupdate処理
    void update_epid_logo(StateManager& manager);
    void update_doujin_notice(StateManager& manager);
    void update_autosave_warn(StateManager& manager);
    void update_title(StateManager& manager);

    bn::sprite_text_generator& text_gen_;
    TitlePhase phase_;
    PhaseStep  step_;
    int frame_counter_;
    int blink_counter_;
    UIManager ui_manager_;
};

#endif // TITLE_STATE_H
