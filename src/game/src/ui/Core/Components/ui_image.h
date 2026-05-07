#ifndef UI_IMAGE_H
#define UI_IMAGE_H

#include "ui_node.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"

class UIImage : public UINode {
public:
    UIImage(const bn::string_view& id, bn::fixed x, bn::fixed y, bn::fixed rotation, bool visible);

    // スプライトの実体をセットする
    void set_sprite(const bn::optional<bn::sprite_ptr>& sprite);

    // オーバーライド
    void set_position(bn::fixed x, bn::fixed y) override;
    void set_visible(bool visible) override;
    void set_rotation(bn::fixed rotation) override;
    void set_scale(bn::fixed scale) override;

private:
    bn::optional<bn::sprite_ptr> sprite_;
    bn::fixed rotation_;
    bn::fixed scale_ = 1.0;
};

#endif // UI_IMAGE_H