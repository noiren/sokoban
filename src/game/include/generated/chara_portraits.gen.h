// AUTO GENERATED FILE. DO NOT EDIT. (generate_chara_portraits.py)
#pragma once

#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "bn_string_view.h"
#include "bn_fixed.h"
#include "bn_sprite_items_spr_ch_mayo_angry_1.h"
#include "bn_sprite_items_spr_ch_mayo_angry_2.h"
#include "bn_sprite_items_spr_ch_mayo_angry_3.h"
#include "bn_sprite_items_spr_ch_mayo_happy_1.h"
#include "bn_sprite_items_spr_ch_mayo_happy_2.h"
#include "bn_sprite_items_spr_ch_mayo_happy_3.h"
#include "bn_sprite_items_spr_ch_mayo_normal_1.h"
#include "bn_sprite_items_spr_ch_mayo_normal_2.h"
#include "bn_sprite_items_spr_ch_mayo_normal_3.h"
#include "bn_sprite_items_spr_ch_mayo_sad_1.h"
#include "bn_sprite_items_spr_ch_mayo_sad_2.h"
#include "bn_sprite_items_spr_ch_mayo_sad_3.h"
#include "bn_sprite_items_spr_ch_mayo_smile_1.h"
#include "bn_sprite_items_spr_ch_mayo_smile_2.h"
#include "bn_sprite_items_spr_ch_mayo_smile_3.h"
#include "bn_sprite_items_spr_ch_mayo_surprised_1.h"
#include "bn_sprite_items_spr_ch_mayo_surprised_2.h"
#include "bn_sprite_items_spr_ch_mayo_surprised_3.h"
#include "bn_sprite_items_spr_ch_mayo_think_1.h"
#include "bn_sprite_items_spr_ch_mayo_think_2.h"
#include "bn_sprite_items_spr_ch_mayo_think_3.h"
#include "bn_sprite_items_spr_ch_riri_normal.h"

namespace chara_portraits {

constexpr int COUNT = 22;

inline bn::optional<bn::sprite_ptr> create_by_index(int index, bn::fixed x, bn::fixed y) {
    if (index == 0) return bn::sprite_items::spr_ch_mayo_angry_1.create_sprite(x, y);
    else if (index == 1) return bn::sprite_items::spr_ch_mayo_angry_2.create_sprite(x, y);
    else if (index == 2) return bn::sprite_items::spr_ch_mayo_angry_3.create_sprite(x, y);
    else if (index == 3) return bn::sprite_items::spr_ch_mayo_happy_1.create_sprite(x, y);
    else if (index == 4) return bn::sprite_items::spr_ch_mayo_happy_2.create_sprite(x, y);
    else if (index == 5) return bn::sprite_items::spr_ch_mayo_happy_3.create_sprite(x, y);
    else if (index == 6) return bn::sprite_items::spr_ch_mayo_normal_1.create_sprite(x, y);
    else if (index == 7) return bn::sprite_items::spr_ch_mayo_normal_2.create_sprite(x, y);
    else if (index == 8) return bn::sprite_items::spr_ch_mayo_normal_3.create_sprite(x, y);
    else if (index == 9) return bn::sprite_items::spr_ch_mayo_sad_1.create_sprite(x, y);
    else if (index == 10) return bn::sprite_items::spr_ch_mayo_sad_2.create_sprite(x, y);
    else if (index == 11) return bn::sprite_items::spr_ch_mayo_sad_3.create_sprite(x, y);
    else if (index == 12) return bn::sprite_items::spr_ch_mayo_smile_1.create_sprite(x, y);
    else if (index == 13) return bn::sprite_items::spr_ch_mayo_smile_2.create_sprite(x, y);
    else if (index == 14) return bn::sprite_items::spr_ch_mayo_smile_3.create_sprite(x, y);
    else if (index == 15) return bn::sprite_items::spr_ch_mayo_surprised_1.create_sprite(x, y);
    else if (index == 16) return bn::sprite_items::spr_ch_mayo_surprised_2.create_sprite(x, y);
    else if (index == 17) return bn::sprite_items::spr_ch_mayo_surprised_3.create_sprite(x, y);
    else if (index == 18) return bn::sprite_items::spr_ch_mayo_think_1.create_sprite(x, y);
    else if (index == 19) return bn::sprite_items::spr_ch_mayo_think_2.create_sprite(x, y);
    else if (index == 20) return bn::sprite_items::spr_ch_mayo_think_3.create_sprite(x, y);
    else if (index == 21) return bn::sprite_items::spr_ch_riri_normal.create_sprite(x, y);
    return bn::optional<bn::sprite_ptr>();
}

inline bn::optional<bn::sprite_ptr> create_by_id(const bn::string_view& image_id, bn::fixed x, bn::fixed y) {
    if (image_id == "mayo_angry_1") return bn::sprite_items::spr_ch_mayo_angry_1.create_sprite(x, y);
    else if (image_id == "mayo_angry_2") return bn::sprite_items::spr_ch_mayo_angry_2.create_sprite(x, y);
    else if (image_id == "mayo_angry_3") return bn::sprite_items::spr_ch_mayo_angry_3.create_sprite(x, y);
    else if (image_id == "mayo_happy_1") return bn::sprite_items::spr_ch_mayo_happy_1.create_sprite(x, y);
    else if (image_id == "mayo_happy_2") return bn::sprite_items::spr_ch_mayo_happy_2.create_sprite(x, y);
    else if (image_id == "mayo_happy_3") return bn::sprite_items::spr_ch_mayo_happy_3.create_sprite(x, y);
    else if (image_id == "mayo_normal_1") return bn::sprite_items::spr_ch_mayo_normal_1.create_sprite(x, y);
    else if (image_id == "mayo_normal_2") return bn::sprite_items::spr_ch_mayo_normal_2.create_sprite(x, y);
    else if (image_id == "mayo_normal_3") return bn::sprite_items::spr_ch_mayo_normal_3.create_sprite(x, y);
    else if (image_id == "mayo_sad_1") return bn::sprite_items::spr_ch_mayo_sad_1.create_sprite(x, y);
    else if (image_id == "mayo_sad_2") return bn::sprite_items::spr_ch_mayo_sad_2.create_sprite(x, y);
    else if (image_id == "mayo_sad_3") return bn::sprite_items::spr_ch_mayo_sad_3.create_sprite(x, y);
    else if (image_id == "mayo_smile_1") return bn::sprite_items::spr_ch_mayo_smile_1.create_sprite(x, y);
    else if (image_id == "mayo_smile_2") return bn::sprite_items::spr_ch_mayo_smile_2.create_sprite(x, y);
    else if (image_id == "mayo_smile_3") return bn::sprite_items::spr_ch_mayo_smile_3.create_sprite(x, y);
    else if (image_id == "mayo_surprised_1") return bn::sprite_items::spr_ch_mayo_surprised_1.create_sprite(x, y);
    else if (image_id == "mayo_surprised_2") return bn::sprite_items::spr_ch_mayo_surprised_2.create_sprite(x, y);
    else if (image_id == "mayo_surprised_3") return bn::sprite_items::spr_ch_mayo_surprised_3.create_sprite(x, y);
    else if (image_id == "mayo_think_1") return bn::sprite_items::spr_ch_mayo_think_1.create_sprite(x, y);
    else if (image_id == "mayo_think_2") return bn::sprite_items::spr_ch_mayo_think_2.create_sprite(x, y);
    else if (image_id == "mayo_think_3") return bn::sprite_items::spr_ch_mayo_think_3.create_sprite(x, y);
    else if (image_id == "riri_normal") return bn::sprite_items::spr_ch_riri_normal.create_sprite(x, y);
    return bn::optional<bn::sprite_ptr>();
}

inline int index_of(const bn::string_view& image_id) {
    if (image_id == "mayo_angry_1") return 0;
    else if (image_id == "mayo_angry_2") return 1;
    else if (image_id == "mayo_angry_3") return 2;
    else if (image_id == "mayo_happy_1") return 3;
    else if (image_id == "mayo_happy_2") return 4;
    else if (image_id == "mayo_happy_3") return 5;
    else if (image_id == "mayo_normal_1") return 6;
    else if (image_id == "mayo_normal_2") return 7;
    else if (image_id == "mayo_normal_3") return 8;
    else if (image_id == "mayo_sad_1") return 9;
    else if (image_id == "mayo_sad_2") return 10;
    else if (image_id == "mayo_sad_3") return 11;
    else if (image_id == "mayo_smile_1") return 12;
    else if (image_id == "mayo_smile_2") return 13;
    else if (image_id == "mayo_smile_3") return 14;
    else if (image_id == "mayo_surprised_1") return 15;
    else if (image_id == "mayo_surprised_2") return 16;
    else if (image_id == "mayo_surprised_3") return 17;
    else if (image_id == "mayo_think_1") return 18;
    else if (image_id == "mayo_think_2") return 19;
    else if (image_id == "mayo_think_3") return 20;
    else if (image_id == "riri_normal") return 21;
    return -1;
}

} // namespace chara_portraits
