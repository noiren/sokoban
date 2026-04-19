#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"
#include "bn_string_view.h"
#include "bn_sprite_text_generator.h"

// 共通インクルード
#include "ui_data_title.h"
#include "ui_data_save_select.h"
#include "ui_data_menu.h"

// 背景スチルを列挙（コード直書き用。JSON側はbg_xxxの文字列名で紐づく）
enum class BgImageID {
    NONE,
    LOGO,
    TITLE,
};

// スプライト列挙
enum class SpriteImageID {
    NONE,
    SPIKI_STAND,
    MAYO_STAND,
};

// UIテキストの状態を保持する構造体
struct RuntimeUIText {
    bn::string_view id;
    int x, y;
    bool blink;
    int blink_interval;
    bool visible;
    bn::string_view text;
    // 表示用スプライトインスタンス
    bn::vector<bn::sprite_ptr, 16> sprites;
};

class UIManager {
public:
    UIManager(bn::sprite_text_generator& text_gen);

    // 古い直接指定API
    void set_bg(BgImageID id);
    void clear_bg();

    void load_screen(const ui_types::ScreenData& screen_data);

    void update(); // 点滅やアニメーション用
    void clear_all();

private:
    void _set_bg_from_string(bn::string_view bg_id);

    bn::sprite_text_generator& text_gen_;
    bn::optional<bn::regular_bg_ptr> bg_;
    int tick_counter_ = 0;

    bn::vector<RuntimeUIText, 16> texts_;
};

#endif // UI_MANAGER_H
