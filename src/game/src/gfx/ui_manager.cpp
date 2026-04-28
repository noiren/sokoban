#include "ui_manager.h"

// Butano自動生成ヘッダ
#include "bn_regular_bg_items_bg_logo.h"
#include "bn_regular_bg_items_bg_title.h"
#include "bn_regular_bg_items_bg_menu_main.h"
#include "bn_regular_bg_items_bg_title_main.h"
#include "bn_sprite_items_spr_dummy.h"
#include "bn_sprite_items_spr_btn_l.h"
#include "bn_sprite_items_spr_btn_r.h"
#include "bn_sprite_items_spr_button_ok.h"
#include "bn_sprite_items_spr_ch_mayo_normal.h"
#include "bn_sprite_items_spr_ch_mayo_sad.h"
#include "bn_sprite_items_spr_ch_mayo_smile.h"
#include "bn_sprite_items_spr_ch_riri_normal.h"
#include "bn_sprite_items_spr_face_icon_mayo.h"
#include "bn_sprite_items_spr_face_icon_riri.h"
#include "bn_sprite_items_spr_icon_clapper.h"
#include "bn_sprite_items_spr_icon_lock.h"
#include "bn_sprite_items_spr_icon_note.h"
#include "bn_sprite_items_spr_icon_pot.h"
#include "bn_sprite_items_spr_icon_pumpkin.h"
#include "bn_sprite_items_spr_icon_ticket.h"
#include "bn_sprite_items_spr_mn_crying.h"
#include "bn_sprite_items_spr_nav_arrow.h"
#include "bn_sprite_items_spr_paper_large.h"
#include "bn_sprite_items_spr_paper_medium.h"
#include "bn_sprite_items_spr_paper_small.h"
#include "bn_sprite_items_spr_paper_tail.h"
#include "bn_sprite_items_spr_photo_corner.h"
#include "bn_sprite_items_spr_press_start.h"
#include "bn_sprite_items_spr_selection_common.h"
#include "bn_sprite_items_spr_selector_gold_bl.h"
#include "bn_sprite_items_spr_selector_gold_br.h"
#include "bn_sprite_items_spr_selector_gold_tl.h"
#include "bn_sprite_items_spr_selector_gold_tr.h"
#include "bn_sprite_items_spr_sparkle.h"
#include "bn_sprite_items_spr_stage_icon_base.h"
#include "bn_sprite_items_spr_stage_icon_cloud.h"
#include "bn_sprite_items_spr_thum_01.h"
#include "bn_sprite_items_spr_thum_02.h"
#include "bn_sprite_items_spr_thum_03.h"

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
    } else if (bg_id == "bg_title_main") {
        bg_ = bn::regular_bg_items::bg_title_main.create_bg(0, 0);
    } else if (bg_id == "bg_menu_main") {
        bg_ = bn::regular_bg_items::bg_menu_main.create_bg(0, 0);
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
            if (img_set == "ui_common") {
                if (img_no == 0) rs.sprite = bn::sprite_items::spr_selection_common.create_sprite(rs.x, rs.y);
                else if (img_no == 1) rs.sprite = bn::sprite_items::spr_selector_gold_tl.create_sprite(rs.x, rs.y);
                else if (img_no == 2) rs.sprite = bn::sprite_items::spr_selector_gold_tr.create_sprite(rs.x, rs.y);
                else if (img_no == 3) rs.sprite = bn::sprite_items::spr_selector_gold_bl.create_sprite(rs.x, rs.y);
                else if (img_no == 4) rs.sprite = bn::sprite_items::spr_selector_gold_br.create_sprite(rs.x, rs.y);
                else if (img_no == 5) rs.sprite = bn::sprite_items::spr_sparkle.create_sprite(rs.x, rs.y);
                else if (img_no == 6) rs.sprite = bn::sprite_items::spr_button_ok.create_sprite(rs.x, rs.y);
                else if (img_no == 7) rs.sprite = bn::sprite_items::spr_nav_arrow.create_sprite(rs.x, rs.y);
                else if (img_no == 8) rs.sprite = bn::sprite_items::spr_photo_corner.create_sprite(rs.x, rs.y);
            }
            else if (img_set == "ui_icons") {
                if (img_no == 0) rs.sprite = bn::sprite_items::spr_icon_lock.create_sprite(rs.x, rs.y);
                else if (img_no == 1) rs.sprite = bn::sprite_items::spr_icon_pumpkin.create_sprite(rs.x, rs.y);
                else if (img_no == 2) rs.sprite = bn::sprite_items::spr_icon_pot.create_sprite(rs.x, rs.y);
                else if (img_no == 3) rs.sprite = bn::sprite_items::spr_icon_note.create_sprite(rs.x, rs.y);
                else if (img_no == 4) rs.sprite = bn::sprite_items::spr_icon_ticket.create_sprite(rs.x, rs.y);
                else if (img_no == 5) rs.sprite = bn::sprite_items::spr_icon_clapper.create_sprite(rs.x, rs.y);
                else if (img_no == 6) rs.sprite = bn::sprite_items::spr_btn_l.create_sprite(rs.x, rs.y);
                else if (img_no == 7) rs.sprite = bn::sprite_items::spr_btn_r.create_sprite(rs.x, rs.y);
            }
            else if (img_set == "ui_paper") {
                if (img_no == 0) rs.sprite = bn::sprite_items::spr_paper_large.create_sprite(rs.x, rs.y);
                else if (img_no == 1) rs.sprite = bn::sprite_items::spr_paper_medium.create_sprite(rs.x, rs.y);
                else if (img_no == 2) rs.sprite = bn::sprite_items::spr_paper_small.create_sprite(rs.x, rs.y);
                else if (img_no == 3) rs.sprite = bn::sprite_items::spr_paper_tail.create_sprite(rs.x, rs.y);
            }
            else if (img_set == "ui_title") {
                if (img_no == 0) rs.sprite = bn::sprite_items::spr_press_start.create_sprite(rs.x, rs.y);
            }
            else if (img_set == "ui_practice") {
                if (img_no == 0) rs.sprite = bn::sprite_items::spr_stage_icon_base.create_sprite(rs.x, rs.y);
                else if (img_no == 1) rs.sprite = bn::sprite_items::spr_stage_icon_cloud.create_sprite(rs.x, rs.y);
            }
            else if (img_set == "chara_portraits") {
                if (img_no == 0) rs.sprite = bn::sprite_items::spr_ch_mayo_normal.create_sprite(rs.x, rs.y);
                else if (img_no == 1) rs.sprite = bn::sprite_items::spr_ch_mayo_smile.create_sprite(rs.x, rs.y);
                else if (img_no == 2) rs.sprite = bn::sprite_items::spr_ch_mayo_sad.create_sprite(rs.x, rs.y);
                else if (img_no == 3) rs.sprite = bn::sprite_items::spr_ch_riri_normal.create_sprite(rs.x, rs.y);
            }
            else if (img_set == "chara_mini") {
                if (img_no == 0) rs.sprite = bn::sprite_items::spr_mn_crying.create_sprite(rs.x, rs.y);
                else if (img_no == 1) rs.sprite = bn::sprite_items::spr_face_icon_mayo.create_sprite(rs.x, rs.y);
                else if (img_no == 2) rs.sprite = bn::sprite_items::spr_face_icon_riri.create_sprite(rs.x, rs.y);
            }
            else if (img_set == "gallery_thum") {
                if (img_no == 0) rs.sprite = bn::sprite_items::spr_thum_01.create_sprite(rs.x, rs.y);
                else if (img_no == 1) rs.sprite = bn::sprite_items::spr_thum_02.create_sprite(rs.x, rs.y);
                else if (img_no == 2) rs.sprite = bn::sprite_items::spr_thum_03.create_sprite(rs.x, rs.y);
            }
            else if (img_set == "dummy") {
                if (img_no == 0) rs.sprite = bn::sprite_items::spr_dummy.create_sprite(rs.x, rs.y);
            }
            else {
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
