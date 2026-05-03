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

// 起動シーケンス全フェーズ
// EPID_LOGO     : "produced by EPID GAMES" ロゴ表示
// DOUJIN_NOTICE : "この作品は同人作品です" 注意書き
// AUTOSAVE_WARN : オートセーブ警告メッセージ
// TITLE_FADEIN  : タイトル画面フェードイン
// TITLE_WAIT    : タイトル画面待機 (PRESS START 点滅)
// TITLE_FADEOUT : タイトル画面フェードアウト → SaveSelectへ
enum class TitlePhase {
    EPID_LOGO_FADEIN,
    EPID_LOGO_WAIT,
    EPID_LOGO_FADEOUT,
    DOUJIN_NOTICE_FADEIN,
    DOUJIN_NOTICE_WAIT,
    DOUJIN_NOTICE_FADEOUT,
    AUTOSAVE_WARN_FADEIN,
    AUTOSAVE_WARN_WAIT,
    AUTOSAVE_WARN_FADEOUT,
    TITLE_FADEIN,
    TITLE_WAIT,
    TITLE_FADEOUT,
};

class TitleState : public State {
public:
    explicit TitleState(bn::sprite_text_generator& text_gen);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

private:
    // フェードを0→1に進める。完了したらtrueを返す
    bool fade_in(int duration);
    // フェードを1→0に進める。完了したらtrueを返す
    bool fade_out(int duration);

    bn::sprite_text_generator& text_gen_;
    TitlePhase phase_;
    int frame_counter_;
    int blink_counter_;
    UIManager ui_manager_;
};

#endif // TITLE_STATE_H
