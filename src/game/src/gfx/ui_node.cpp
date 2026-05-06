#include "ui_node.h"

UINode::UINode(const bn::string_view& id, bn::fixed x, bn::fixed y, bool visible)
    : id_(id), x_(x), y_(y), visible_(visible) {}

void UINode::set_position(bn::fixed x, bn::fixed y) {
    x_ = x;
    y_ = y;
}

void UINode::set_visible(bool visible) {
    visible_ = visible;
}

void UINode::set_rotation(bn::fixed rotation) {
    // 派生クラスで実装
}

void UINode::set_scale(bn::fixed scale) {
    // 派生クラスで実装
}