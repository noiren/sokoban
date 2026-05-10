#pragma once

#include "bn_core.h"
#include "ui_types.h"

namespace AnimMath {
    // 補間結果を受け取るための構造体
    struct Transform {
        bn::fixed x;
        bn::fixed y;
        bn::fixed rot;
        bn::fixed scale;
    };

    // イージング計算（内部利用または個別利用向け）
    bn::fixed calculate_ease(bn::fixed t, ui_types::EaseType type);

    // AnimPresetから指定されたフレーム（timer）のTransformを算出する
    Transform evaluate(const ui_types::AnimPreset* preset, int timer);

    // 【重要】アフィン（回転・拡縮）を利用しているキーフレームが含まれているかを判定
    bool requires_affine(const ui_types::AnimPreset* preset);
}
