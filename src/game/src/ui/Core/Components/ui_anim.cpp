#include "ui_anim.h"

UIAnim::UIAnim(const bn::string_view& id) : UINode(id, 0, 0, true) {}

void UIAnim::set_preset(const ui_types::AnimPreset* preset) {
    preset_ = preset;
}

void UIAnim::add_target(UINode* node) {
    if (node) {
        // UIAnim追加時点での座標を「本来の基準位置」として記憶する
        targets_.push_back({node, node->get_x(), node->get_y(), bn::fixed(0), bn::fixed(1.0)});
    }
}

void UIAnim::play() {
    if (!preset_ || preset_->keyframe_count == 0) return;
    is_playing_ = true;
    timer_ = 0;
}

void UIAnim::reset_timer() {
    timer_ = 0;
}

bn::fixed UIAnim::_calculate_ease(bn::fixed t, ui_types::EaseType type) {
    if (t <= 0) return 0;
    if (t >= 1) return 1;

    bn::fixed t2 = t * t;
    bn::fixed t3 = t2 * t;

    switch (type) {
        case ui_types::EaseType::LINEAR: 
            return t;
        case ui_types::EaseType::EASE_IN: 
            return t2;
        case ui_types::EaseType::EASE_OUT: 
            return t * (bn::fixed(2) - t);
        case ui_types::EaseType::EASE_IN_OUT: 
            if (t < bn::fixed(0.5)) return bn::fixed(2) * t2;
            else return bn::fixed(-1) + (bn::fixed(4) - bn::fixed(2) * t) * t;
        case ui_types::EaseType::CUBIC_IN: 
            return t3;
        case ui_types::EaseType::CUBIC_OUT: {
            bn::fixed f = t - bn::fixed(1);
            return f * f * f + bn::fixed(1);
        }
        case ui_types::EaseType::CUBIC_IN_OUT: {
            if (t < bn::fixed(0.5)) return bn::fixed(4) * t3;
            else {
                bn::fixed f = (bn::fixed(2) * t) - bn::fixed(2);
                return bn::fixed(0.5) * f * f * f + bn::fixed(1);
            }
        }
        case ui_types::EaseType::BACK_IN: {
            const bn::fixed s(1.70158f);
            return t * t * ((s + 1) * t - s);
        }
        case ui_types::EaseType::BACK_OUT: {
            const bn::fixed s(1.70158f);
            bn::fixed f = t - bn::fixed(1);
            return (f * f * ((s + 1) * f + s) + bn::fixed(1));
        }
        case ui_types::EaseType::BOUNCE_OUT: {
            if (t < bn::fixed(1.0f / 2.75f)) {
                return bn::fixed(7.5625f) * t * t;
            } else if (t < bn::fixed(2.0f / 2.75f)) {
                t -= bn::fixed(1.5f / 2.75f);
                return bn::fixed(7.5625f) * t * t + bn::fixed(0.75f);
            } else if (t < bn::fixed(2.5f / 2.75f)) {
                t -= bn::fixed(2.25f / 2.75f);
                return bn::fixed(7.5625f) * t * t + bn::fixed(0.9375f);
            } else {
                t -= bn::fixed(2.625f / 2.75f);
                return bn::fixed(7.5625f) * t * t + bn::fixed(0.984375f);
            }
        }
    }
    return t;
}

void UIAnim::update() {
    if (!is_playing_ || !preset_ || preset_->keyframe_count == 0) return;

    timer_++;

    const ui_types::AnimKeyframe* kf_start = &preset_->keyframes[0];
    const ui_types::AnimKeyframe* kf_end = kf_start;

    // 現在のタイマーがどのキーフレーム間にあるか探す
    for (int i = 0; i < preset_->keyframe_count - 1; ++i) {
        if (timer_ >= preset_->keyframes[i].frame && timer_ <= preset_->keyframes[i+1].frame) {
            kf_start = &preset_->keyframes[i];
            kf_end = &preset_->keyframes[i+1];
            break;
        }
    }

    // キーフレーム間の進捗率
    int frame_diff = kf_end->frame - kf_start->frame;
    bn::fixed progress = 1;
    if (frame_diff > 0) {
        progress = bn::fixed(timer_ - kf_start->frame) / frame_diff;
    }
    if (progress > 1) progress = 1;

    // イージング適用
    bn::fixed ease_val = _calculate_ease(progress, kf_start->ease_type);

    // 線形補間 (Lerp)
    bn::fixed cur_x = bn::fixed(kf_start->x) + (bn::fixed(kf_end->x) - bn::fixed(kf_start->x)) * ease_val;
    bn::fixed cur_y = bn::fixed(kf_start->y) + (bn::fixed(kf_end->y) - bn::fixed(kf_start->y)) * ease_val;
    bn::fixed cur_rot = bn::fixed(kf_start->rot) + (bn::fixed(kf_end->rot) - bn::fixed(kf_start->rot)) * ease_val;
    bn::fixed cur_scale = bn::fixed(kf_start->scale) + (bn::fixed(kf_end->scale) - bn::fixed(kf_start->scale)) * ease_val;

    // 子ノードへ一斉適用
    for (auto& t : targets_) {
        // 座標は「オフセット（加算）」
        t.node->set_position(t.base_x + cur_x, t.base_y + cur_y);
        // 回転とスケールは「上書き」
        t.node->set_rotation(cur_rot);
        t.node->set_scale(cur_scale);
        
        if (visible_) t.node->set_visible(true);
    }

    if (timer_ >= preset_->duration_frames) {
        is_playing_ = false; // 再生終了
    }
}