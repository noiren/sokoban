#ifndef UI_ANIM_H
#define UI_ANIM_H

#include "ui_types.h"
#include "bn_vector.h"
#include "ui_node.h"

class UIAnim : public UINode {
public:
    UIAnim(const bn::string_view& id);
    void set_preset(const ui_types::AnimPreset* preset);
    void add_target(UINode* node);
    void play();
    void reset_timer();
    void update() override;

private:
    struct TargetEntry {
        UINode* node;
        bn::fixed base_x;
        bn::fixed base_y;
        bn::fixed base_rot;
        bn::fixed base_scale;
    };

    bn::vector<TargetEntry, 8> targets_;
    const ui_types::AnimPreset* preset_ = nullptr;
    int timer_ = 0;
    bool is_playing_ = false;
};

#endif