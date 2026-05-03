#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"
#include "bn_string_view.h"
#include "bn_string.h"
#include "bn_sprite_text_generator.h"

// 共通インクルード
#include "ui_types.h"

// 背景スチルを列挙（コード直書き用。JSON側はbg_xxxの文字列名で紐づく）
enum class BgImageID {
    NONE,
    LOGO,
    TITLE,
};

#include "bn_fixed.h"

// スプライト列挙
enum class SpriteImageID {
    NONE,
    SPIKI_STAND,
    MAYO_STAND,
};

// UIスプライトの実行時状態
struct RuntimeUISprite {
    bn::string_view id;
    bn::fixed x, y;
    bool visible;
    bn::optional<bn::sprite_ptr> sprite;
};

// UIテキストの状態を保持する構造体
struct RuntimeUIText {
    bn::string<32> id;           // owning string (ID検索用)
    bn::fixed x, y;
    bool blink;
    int blink_interval;
    bool visible;
    bn::string<64> text;         // owning string (set_textで書き換えられる)
    bool dirty = true;           // true のとき次フレームで再生成
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

    // 動的UI操作用API
    void set_sprite_visible(const bn::string_view& id, bool visible);
    void set_sprite_position(const bn::string_view& id, bn::fixed x, bn::fixed y);
    void set_sprite_image(const bn::string_view& id, const bn::string_view& image_set, int image_no);

    void set_text(const bn::string_view& id, const bn::string_view& text);
    void set_text_visible(const bn::string_view& id, bool visible);

    void update(); // 点滅やアニメーション用
    void clear_all();

private:
    void _set_bg_from_string(bn::string_view bg_id);
    bn::optional<bn::sprite_ptr> _create_sprite_from_set(const bn::string_view& img_set, int img_no, bn::fixed x, bn::fixed y);

    bn::sprite_text_generator& text_gen_;
    bn::optional<bn::regular_bg_ptr> bg_;
    int tick_counter_ = 0;

    bn::vector<RuntimeUISprite, 16> sprites_;
    bn::vector<RuntimeUIText, 16> texts_;
};

#endif // UI_MANAGER_H
