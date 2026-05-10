#include "ui_anim.h"
#include "ui_anim_math.h" // 分離した共通Mathを利用

UIAnim::UIAnim(const bn::string_view& id) : UINode(id, 0, 0, true) {}

void UIAnim::set_preset(const ui_types::AnimPreset* preset) { preset_ = preset; }

void UIAnim::add_target(UINode* node) {
    if (node) {
        targets_.push_back({node, node->get_x(), node->get_y(), bn::fixed(0), bn::fixed(1.0)});
    }
}

void UIAnim::play() {
    if (!preset_ || preset_->keyframe_count == 0) return;
    is_playing_ = true;
    timer_ = 0;
}

void UIAnim::reset_timer() { timer_ = 0; }

void UIAnim::update() {
    if (!is_playing_ || !preset_ || preset_->keyframe_count == 0) return;
    timer_++;

    // 共通の補間ロジックを呼び出す
    AnimMath::Transform t = AnimMath::evaluate(preset_, timer_);

    for (auto& target : targets_) {
        target.node->set_position(target.base_x + t.x, target.base_y + t.y);
        target.node->set_rotation(t.rot);
        target.node->set_scale(t.scale);
        if (visible_) target.node->set_visible(true);
    }

    if (timer_ >= preset_->duration_frames) { 
        is_playing_ = false; 
    }
}