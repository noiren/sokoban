#ifndef UI_NODE_H
#define UI_NODE_H

#include "bn_string_view.h"
#include "bn_fixed.h"

class UINode {
public:
    UINode(const bn::string_view& id, bn::fixed x, bn::fixed y, bool visible);
    virtual ~UINode() = default;

    // 共通の操作
    virtual void set_position(bn::fixed x, bn::fixed y);
    virtual void set_visible(bool visible);
    
    // アニメーション用の拡張操作
    virtual void set_rotation(bn::fixed rotation);
    virtual void set_scale(bn::fixed scale);
    virtual void set_bg_priority(int /*priority*/) {}

    // 毎フレームの更新処理（オーバーライド用）
    virtual void update() {}

    // ゲッター
    bn::string_view get_id() const { return id_; }
    bn::fixed get_x() const { return x_; }
    bn::fixed get_y() const { return y_; }
    bool is_visible() const { return visible_; }

protected:
    bn::string_view id_;
    bn::fixed x_;
    bn::fixed y_;
    bool visible_;
};

#endif // UI_NODE_H