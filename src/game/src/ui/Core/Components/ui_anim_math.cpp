#include "ui_anim_math.h"

namespace AnimMath {

bn::fixed calculate_ease(bn::fixed t, ui_types::EaseType type) {
    if (t <= 0) return 0;
    if (t >= 1) return 1;
    
    bn::fixed t2 = t * t;
    bn::fixed t3 = t2 * t;
    
    switch (type) {
        case ui_types::EaseType::LINEAR: return t;
        case ui_types::EaseType::EASE_IN: return t2;
        case ui_types::EaseType::EASE_OUT: return t * (bn::fixed(2) - t);
        case ui_types::EaseType::EASE_IN_OUT: 
            if (t < bn::fixed(0.5)) return bn::fixed(2) * t2;
            else return bn::fixed(-1) + (bn::fixed(4) - bn::fixed(2) * t) * t;
        case ui_types::EaseType::CUBIC_IN: return t3;
        case ui_types::EaseType::CUBIC_OUT: {
            bn::fixed f = t - bn::fixed(1); return f * f * f + bn::fixed(1);
        }
        case ui_types::EaseType::CUBIC_IN_OUT: {
            if (t < bn::fixed(0.5)) return bn::fixed(4) * t3;
            else {
                bn::fixed f = (bn::fixed(2) * t) - bn::fixed(2);
                return bn::fixed(0.5) * f * f * f + bn::fixed(1);
            }
        }
        case ui_types::EaseType::BACK_IN: {
            const bn::fixed s(1.70158f); return t * t * ((s + 1) * t - s);
        }
        case ui_types::EaseType::BACK_OUT: {
            const bn::fixed s(1.70158f); bn::fixed f = t - bn::fixed(1);
            return (f * f * ((s + 1) * f + s) + bn::fixed(1));
        }
        case ui_types::EaseType::BOUNCE_OUT: {
            if (t < bn::fixed(1.0f / 2.75f)) { return bn::fixed(7.5625f) * t * t; }
            else if (t < bn::fixed(2.0f / 2.75f)) {
                t -= bn::fixed(1.5f / 2.75f); return bn::fixed(7.5625f) * t * t + bn::fixed(0.75f);
            }
            else if (t < bn::fixed(2.5f / 2.75f)) {
                t -= bn::fixed(2.25f / 2.75f); return bn::fixed(7.5625f) * t * t + bn::fixed(0.9375f);
            }
            else {
                t -= bn::fixed(2.625f / 2.75f); return bn::fixed(7.5625f) * t * t + bn::fixed(0.984375f);
            }
        }
    }
    return t;
}

Transform evaluate(const ui_types::AnimPreset* preset, int timer) {
    Transform result = {0, 0, 0, 1}; // デフォルト値
    if (!preset || preset->keyframe_count == 0) return result;

    // タイマーが0以下の場合は最初のキーフレームを返す
    if (timer <= preset->keyframes[0].frame) {
        const auto& kf = preset->keyframes[0];
        return {bn::fixed(kf.x), bn::fixed(kf.y), bn::fixed(kf.rot), bn::fixed(kf.scale)};
    }

    // タイマーが最後のキーフレームを超えている場合は最後のキーフレームを返す
    int last_idx = preset->keyframe_count - 1;
    if (timer >= preset->keyframes[last_idx].frame) {
        const auto& kf = preset->keyframes[last_idx];
        return {bn::fixed(kf.x), bn::fixed(kf.y), bn::fixed(kf.rot), bn::fixed(kf.scale)};
    }

    // 該当するキーフレーム区間を探す
    const ui_types::AnimKeyframe* kf_start = &preset->keyframes[0];
    const ui_types::AnimKeyframe* kf_end = kf_start;
    
    for (int i = 0; i < preset->keyframe_count - 1; ++i) {
        if (timer >= preset->keyframes[i].frame && timer <= preset->keyframes[i+1].frame) {
            kf_start = &preset->keyframes[i];
            kf_end = &preset->keyframes[i+1];
            break;
        }
    }

    int frame_diff = kf_end->frame - kf_start->frame;
    bn::fixed progress = 1;
    if (frame_diff > 0) { 
        progress = bn::fixed(timer - kf_start->frame) / frame_diff; 
    }
    if (progress > 1) progress = 1;

    bn::fixed ease_val = calculate_ease(progress, kf_start->ease_type);

    result.x = bn::fixed(kf_start->x) + (bn::fixed(kf_end->x) - bn::fixed(kf_start->x)) * ease_val;
    result.y = bn::fixed(kf_start->y) + (bn::fixed(kf_end->y) - bn::fixed(kf_start->y)) * ease_val;
    result.rot = bn::fixed(kf_start->rot) + (bn::fixed(kf_end->rot) - bn::fixed(kf_start->rot)) * ease_val;
    result.scale = bn::fixed(kf_start->scale) + (bn::fixed(kf_end->scale) - bn::fixed(kf_start->scale)) * ease_val;

    return result;
}

bool requires_affine(const ui_types::AnimPreset* preset) {
    if (!preset) return false;
    // 1つでも回転かスケール変更が含まれていれば true を返す
    for (int i = 0; i < preset->keyframe_count; ++i) {
        if (preset->keyframes[i].rot != 0.0f || preset->keyframes[i].scale != 1.0f) {
            return true;
        }
    }
    return false;
}

} // namespace AnimMath
