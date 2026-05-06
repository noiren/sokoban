#ifndef UI_ANIM_H
#define UI_ANIM_H

#include "ui_node.h"
#include "ui_types.h"
#include "bn_vector.h"
#include "bn_fixed.h"

class UIAnim : public UINode {
public:
    UIAnim(const bn::string_view& id);

    void set_preset(const ui_types::AnimPreset* preset);
    void add_target(UINode* node);

    void play();
    void reset_timer();
    void update() override;

private:
    struct TargetInfo {
        UINode* node;
        bn::fixed base_x;
        bn::fixed base_y;
        bn::fixed base_rot;
        bn::fixed base_scale;
    };

    bn::fixed _calculate_ease(bn::fixed t, ui_types::EaseType type);

    const ui_types::AnimPreset* preset_ = nullptr;
    bn::vector<TargetInfo, 16> targets_;

    bool is_playing_ = false;
    int timer_ = 0;
};

#endif // UI_ANIM_H