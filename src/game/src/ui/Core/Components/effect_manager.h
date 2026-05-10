#ifndef EFFECT_MANAGER_H
#define EFFECT_MANAGER_H

#include "bn_core.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "ui_types.h"

// 1エフェクトの状態
struct Effect {
    bool active = false;
    bn::optional<bn::sprite_ptr> sprite;
    bn::optional<bn::sprite_animate_action<4>> frame_anim; 
    const ui_types::AnimPreset* preset = nullptr;
    
    int timer = 0;
    bn::fixed base_x;
    bn::fixed base_y;
    
    // アフィンアニメーション（回転・拡縮）を使うかどうかのフラグ
    bool use_affine = false; 
};

class EffectManager {
private:
    // OAM(128)とアフィン(32)を考慮した安全な最大数
    static constexpr int MAX_EFFECTS = 32; 
    bn::vector<Effect, MAX_EFFECTS> effects_;

public:
    EffectManager();

    // エフェクト生成
    // 例: spawn(bn::sprite_items::eff_sparkle, 10, 20, &ui_data::anim_sparkle);
    void spawn(const bn::sprite_item& sprite_item, 
               bn::fixed x, bn::fixed y, 
               const ui_types::AnimPreset* preset = nullptr);

    void update();
};

#endif
