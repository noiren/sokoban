#ifndef MENU_STATE_H
#define MENU_STATE_H

#include "state/state.h"
#include "audio/sound_manager.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

// メインメニューの選択肢
// 設計書に合わせてプラクティスとギャラリーを追加
// DEBUG は非表示（SELECTボタンで隠しアクセス）
enum class MenuItem {
    STORY,
    PRACTICE,
    ENDLESS,
    GALLERY,
    SETTINGS,
    DEBUG,   // 非表示隠しコマンド
    COUNT
};

// 表示する項目数（DEBUGを除く）
constexpr int VISIBLE_MENU_COUNT = 5;

class MenuState : public State {
public:
    MenuState(bn::sprite_text_generator& text_gen, SoundManager& sound);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

    MenuItem last_selected() const { return last_selected_; }

private:
    void draw_menu();

    bn::sprite_text_generator& text_gen_;
    SoundManager& sound_;
    int cursor_;
    MenuItem last_selected_;
    bn::vector<bn::sprite_ptr, 64> sprites_;
};

#endif // MENU_STATE_H
