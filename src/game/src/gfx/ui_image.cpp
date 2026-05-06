#include "ui_image.h"

UIImage::UIImage(const bn::string_view& id, bn::fixed x, bn::fixed y, bn::fixed rotation, bool visible)
    : UINode(id, x, y, visible), rotation_(rotation) {}

void UIImage::set_sprite(const bn::optional<bn::sprite_ptr>& sprite) {
    sprite_ = sprite;
    if (sprite_) {
        sprite_->set_position(x_, y_);
        sprite_->set_rotation_angle(rotation_);
        sprite_->set_scale(scale_);
        sprite_->set_visible(visible_);
    }
}

void UIImage::set_position(bn::fixed x, bn::fixed y) {
    UINode::set_position(x, y);
    if (sprite_) {
        sprite_->set_position(x_, y_);
    }
}

void UIImage::set_visible(bool visible) {
    UINode::set_visible(visible);
    if (sprite_) {
        sprite_->set_visible(visible_);
    }
}

void UIImage::set_rotation(bn::fixed rotation) {
    rotation_ = rotation;
    if (sprite_) {
        sprite_->set_rotation_angle(rotation_);
    }
}

void UIImage::set_scale(bn::fixed scale) {
    scale_ = scale;
    if (sprite_) {
        sprite_->set_scale(scale_);
    }
}