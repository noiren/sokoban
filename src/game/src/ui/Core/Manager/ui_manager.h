#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "bn_pool.h"
#include "bn_vector.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string_view.h"
#include "bn_fixed.h"

#include "ui_types.h"
#include "ui/Core/Components/ui_node.h"
#include "ui/Core/Components/ui_image.h"
#include "ui/Core/Components/ui_text.h"
#include "ui/Core/Components/ui_anim.h"

class UIManager {
public:
    UIManager(bn::sprite_text_generator& text_gen);

    void load_screen(const ui_types::ScreenData& screen_data);
    void update();
    void clear_all();

    UIImage* get_image(const bn::string_view& id);
    UIText* get_text(const bn::string_view& id);
    UIAnim* get_anim(const bn::string_view& id); // 追加

    void change_sprite_image(UIImage* node, const bn::string_view& image_set, int image_no);

    // 画像 ID（文字列）から直接スプライトを切り替える (イベント用)
    void change_sprite_image_by_id(UIImage* node, const bn::string_view& image_id);

private:
    void clear_bg();
    void _set_bg_from_string(bn::string_view bg_id);
    bn::optional<bn::sprite_ptr> _create_sprite_from_set(const bn::string_view& img_set, int img_no, bn::fixed x, bn::fixed y);

    bn::sprite_text_generator& text_gen_;
    bn::optional<bn::regular_bg_ptr> bg_;

    bn::pool<UIImage, 16> image_pool_;
    bn::pool<UIText, 16> text_pool_;
    bn::pool<UIAnim, 8> anim_pool_; // アニメーション用プール

    bn::vector<UIImage*, 16> images_;
    bn::vector<UIText*, 16> texts_;
    bn::vector<UIAnim*, 8> anims_;  // アニメーション用リスト
    bn::vector<UINode*, 40> nodes_;
};

#endif // UI_MANAGER_H