#include "ui_manager.h"

// Butano自動生成ヘッダ
#include "bn_regular_bg_items_bg_logo.h"
#include "bn_regular_bg_items_bg_title.h"
#include "bn_sprite_items_spr_dummy.h"
#include "bn_sprite_items_spr_menu_paper.h"
#include "bn_sprite_items_spr_menu_icon_story.h"
#include "bn_sprite_items_spr_menu_icon_practice.h"

UIManager::UIManager(bn::sprite_text_generator& text_gen) : text_gen_(text_gen) {
}

void UIManager::set_bg(BgImageID id) {
    switch (id) {
        case BgImageID::LOGO:
            bg_ = bn::regular_bg_items::bg_logo.create_bg(0, 0);
            break;
        case BgImageID::TITLE:
            bg_ = bn::regular_bg_items::bg_title.create_bg(0, 0);
            break;
        case BgImageID::NONE:
        default:
            clear_bg();
            break;
    }
}

void UIManager::clear_bg() {
    bg_.reset();
}

void UIManager::clear_all() {
    clear_bg();
    sprites_.clear();
    texts_.clear();
}

void UIManager::_set_bg_from_string(bn::string_view bg_id) {
    if (bg_id == "bg_logo") {
        bg_ = bn::regular_bg_items::bg_logo.create_bg(0, 0);
    } else if (bg_id == "bg_title") {
        bg_ = bn::regular_bg_items::bg_title.create_bg(0, 0);
    } else {
        clear_bg();
    }
}

void UIManager::update() {
    tick_counter_++;

    // テキストの点滅や再描画制御
    for (auto& t : texts_) {
        if (!t.visible) {
            t.sprites.clear();
            continue;
        }

        bool draw_now = true;
        if (t.blink && t.blink_interval > 0) {
            if ((tick_counter_ / t.blink_interval) % 2 == 1) {
                draw_now = false;
            }
        }

        if (draw_now) {
            if (t.sprites.empty()) {
                text_gen_.set_center_alignment();
                text_gen_.generate(t.x, t.y, t.text, t.sprites);
            }
        } else {
            t.sprites.clear();
        }
    }
}

void UIManager::load_screen(const ui_types::ScreenData& screen_data) {
    clear_all();

    // 背景のセット
    _set_bg_from_string(screen_data.bg_image_id);

    // スプライトのセット
    for(int i = 0; i < screen_data.sprite_count; ++i) {
        const auto& s = screen_data.sprites[i];
        RuntimeUISprite rs;
        rs.id = s.id;
        rs.x = bn::fixed(s.x);
        rs.y = bn::fixed(s.y);
        rs.visible = s.visible;
        
        bn::string_view img_set = s.image_set;
        int img_no = s.image_no;
        if (rs.visible) {
            if (img_set == "menu_items") {
                if (img_no == 0) rs.sprite = bn::sprite_items::spr_menu_paper.create_sprite(rs.x, rs.y);
                else if (img_no == 1) rs.sprite = bn::sprite_items::spr_menu_icon_story.create_sprite(rs.x, rs.y);
                else if (img_no == 2) rs.sprite = bn::sprite_items::spr_menu_icon_practice.create_sprite(rs.x, rs.y);
            } else if (img_set == "dummy") {
                if (img_no == 0) rs.sprite = bn::sprite_items::spr_dummy.create_sprite(rs.x, rs.y);
            } else {
                // Fallback or missing map
                // rs.sprite = bn::sprite_items::spr_dummy.create_sprite(rs.x, rs.y);
            }
        }
        sprites_.push_back(rs);
    }

    // テキスト要素のセット
    for(int i = 0; i < screen_data.text_count; ++i) {
        const auto& t = screen_data.texts[i];
        RuntimeUIText rt;
        rt.id = t.id;
        rt.x = bn::fixed(t.x);
        rt.y = bn::fixed(t.y);
        rt.blink = t.blink;
        rt.blink_interval = t.blink_interval;
        rt.visible = t.visible;
        rt.text = t.text;
        
        texts_.push_back(rt);
    }
}
