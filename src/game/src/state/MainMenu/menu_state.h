#ifndef MENU_STATE_H
#define MENU_STATE_H

#include "state/state.h"
#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "ui/Core/Manager/ui_manager.h"
#include "ui/MainMenu/main_menu_ui_view.h"

// メニューの項目定義
enum class MenuItem {
    STORY,
    PRACTICE,
    ENDLESS,
    GALLERY,
    SETTINGS,
    DEBUG,   // 非表示隠しコマンド
    COUNT
};

constexpr int VISIBLE_MENU_COUNT = 5;

// メニュー画面の内部フェーズ（現在はMAINのみですが、今後の拡張用にCOUNTを設ける）
enum class MenuPhase {
    MAIN,
    COUNT
};

class MenuState : public State {
public:
    MenuState();

    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    // 汎用フェーズ遷移関数
    void change_phase(MenuPhase next);

    // ==========================================
    // 各フェーズの処理関数群
    // ==========================================
    // メインメニュー
    void enter_main();
    void update_main(StateManager& sm, SharedContext& ctx);
    void exit_main();

    // ==========================================
    // 関数ポインタ・テーブル定義
    // ==========================================
    using EnterExitFunc = void (MenuState::*)();
    using UpdateFunc = void (MenuState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];

    // ==========================================
    // メンバ変数
    // ==========================================
    bn::optional<UIManager>      ui_manager_;
    bn::optional<MainMenuUIView> view_;

    MenuPhase phase_;
    PhaseStep step_;

    int       cursor_;
    MenuItem  last_selected_;
    int       wait_timer_; // 遷移前のアニメーション待機等に使用

    // 現在の各メニューの解禁状況を保持する配列
    bool unlocked_flags_[VISIBLE_MENU_COUNT];
};

#endif // MENU_STATE_H