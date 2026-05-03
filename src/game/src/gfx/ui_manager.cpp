#include "ui_manager.h"

// Butano自動生成ヘッダ
#include "bn_regular_bg_items_stl_attention.h"
#include "bn_regular_bg_items_stl_logo.h"
#include "bn_regular_bg_items_stl_title.h"
#include "bn_regular_bg_items_still_gallerymenu.h"
#include "bn_regular_bg_items_still_bgm.h"
#include "bn_regular_bg_items_still_se.h"
#include "bn_regular_bg_items_still_event.h"
#include "bn_regular_bg_items_still_viewbustup.h"
#include "bn_regular_bg_items_still_viewstill.h"
#include "bn_regular_bg_items_still_mainmenu.h"
#include "bn_regular_bg_items_still_practice.h"
#include "bn_regular_bg_items_still_saveattention.h"
#include "bn_regular_bg_items_still_sokoban_main.h"
#include "bn_regular_bg_items_gba_event.h"
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
    clear_bg(); // Currently unused explicitly, logic moved to JSON
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
    if (bg_id == "stl_logo") {
        bg_ = bn::regular_bg_items::stl_logo.create_bg(0, 0);
    } else if (bg_id == "stl_title") {
        bg_ = bn::regular_bg_items::stl_title.create_bg(0, 0);
    } else if (bg_id == "stl_attention") {
        bg_ = bn::regular_bg_items::stl_attention.create_bg(0, 0);
    } else if (bg_id == "still_mainmenu") {
        bg_ = bn::regular_bg_items::still_mainmenu.create_bg(0, 0);
    } else if (bg_id == "still_practice") {
        bg_ = bn::regular_bg_items::still_practice.create_bg(0, 0);
    } else if (bg_id == "still_sokoban_main") {
        bg_ = bn::regular_bg_items::still_sokoban_main.create_bg(0, 0);
    } else if (bg_id == "still_event") {
        bg_ = bn::regular_bg_items::still_event.create_bg(0, 0);
    } else if (bg_id == "gba_event") {
        bg_ = bn::regular_bg_items::gba_event.create_bg(0, 0);
    } else if (bg_id == "still_gallerymenu") {
        bg_ = bn::regular_bg_items::still_gallerymenu.create_bg(0, 0);
    } else if (bg_id == "still_bgm") {
        bg_ = bn::regular_bg_items::still_bgm.create_bg(0, 0);
    } else if (bg_id == "still_se") {
        bg_ = bn::regular_bg_items::still_se.create_bg(0, 0);
    } else if (bg_id == "still_viewbustup") {
        bg_ = bn::regular_bg_items::still_viewbustup.create_bg(0, 0);
    } else if (bg_id == "still_viewstill") {
        bg_ = bn::regular_bg_items::still_viewstill.create_bg(0, 0);
    } else if (bg_id == "still_saveattention") {
        bg_ = bn::regular_bg_items::still_saveattention.create_bg(0, 0);
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
            t.dirty = false;
            continue;
        }

        // blink制御: 点滅期間中はスプライトを消すだけでdirtyは触らない
        bool draw_now = true;
        if (t.blink && t.blink_interval > 0) {
            if ((tick_counter_ / t.blink_interval) % 2 == 1) {
                draw_now = false;
            }
        }

        if (draw_now) {
            // dirty のときのみ再生成（毎フレームgenerateを防止）
            if (t.dirty && !t.text.empty()) {
                t.sprites.clear();
                text_gen_.set_center_alignment();
                text_gen_.generate(t.x, t.y, t.text, t.sprites);
                t.dirty = false;
            } else if (t.dirty) {
                // textが空: spritesだけクリア
                t.sprites.clear();
                t.dirty = false;
            }
        } else {
            // blink点滅オフ期間: spritesを消し、dirty=trueにしておく(点滅後再描画)
            if (!t.sprites.empty()) {
                t.sprites.clear();
                t.dirty = true;
            }
        }
    }
}

bn::optional<bn::sprite_ptr> UIManager::_create_sprite_from_set(const bn::string_view& img_set, int img_no, bn::fixed x, bn::fixed y) {
    if (img_set == "ui_common") {
        if (img_no == 0) return bn::sprite_items::spr_selection_common.create_sprite(x, y);
        else if (img_no == 1) return bn::sprite_items::spr_selector_gold_tl.create_sprite(x, y);
        else if (img_no == 2) return bn::sprite_items::spr_selector_gold_tr.create_sprite(x, y);
        else if (img_no == 3) return bn::sprite_items::spr_selector_gold_bl.create_sprite(x, y);
        else if (img_no == 4) return bn::sprite_items::spr_selector_gold_br.create_sprite(x, y);
        else if (img_no == 5) return bn::sprite_items::spr_sparkle.create_sprite(x, y);
        else if (img_no == 6) return bn::sprite_items::spr_button_ok.create_sprite(x, y);
        else if (img_no == 7) return bn::sprite_items::spr_nav_arrow.create_sprite(x, y);
        else if (img_no == 8) return bn::sprite_items::spr_photo_corner.create_sprite(x, y);
    }
    else if (img_set == "ui_icons") {
        if (img_no == 0) return bn::sprite_items::spr_icon_lock.create_sprite(x, y);
        else if (img_no == 1) return bn::sprite_items::spr_icon_pumpkin.create_sprite(x, y);
        else if (img_no == 2) return bn::sprite_items::spr_icon_pot.create_sprite(x, y);
        else if (img_no == 3) return bn::sprite_items::spr_icon_note.create_sprite(x, y);
        else if (img_no == 4) return bn::sprite_items::spr_icon_ticket.create_sprite(x, y);
        else if (img_no == 5) return bn::sprite_items::spr_icon_clapper.create_sprite(x, y);
        else if (img_no == 6) return bn::sprite_items::spr_btn_l.create_sprite(x, y);
        else if (img_no == 7) return bn::sprite_items::spr_btn_r.create_sprite(x, y);
    }
    else if (img_set == "ui_paper") {
        if (img_no == 0) return bn::sprite_items::spr_paper_large.create_sprite(x, y);
        else if (img_no == 1) return bn::sprite_items::spr_paper_medium.create_sprite(x, y);
        else if (img_no == 2) return bn::sprite_items::spr_paper_small.create_sprite(x, y);
        else if (img_no == 3) return bn::sprite_items::spr_paper_tail.create_sprite(x, y);
    }
    else if (img_set == "ui_title") {
        if (img_no == 0) return bn::sprite_items::spr_press_start.create_sprite(x, y);
    }
    else if (img_set == "ui_practice") {
        if (img_no == 0) return bn::sprite_items::spr_stage_icon_base.create_sprite(x, y);
        else if (img_no == 1) return bn::sprite_items::spr_stage_icon_cloud.create_sprite(x, y);
    }
    else if (img_set == "chara_portraits") {
        if (img_no == 0) return bn::sprite_items::spr_ch_mayo_normal.create_sprite(x, y);
        else if (img_no == 1) return bn::sprite_items::spr_ch_mayo_smile.create_sprite(x, y);
        else if (img_no == 2) return bn::sprite_items::spr_ch_mayo_sad.create_sprite(x, y);
        else if (img_no == 3) return bn::sprite_items::spr_ch_riri_normal.create_sprite(x, y);
    }
    else if (img_set == "chara_mini") {
        if (img_no == 0) return bn::sprite_items::spr_mn_crying.create_sprite(x, y);
        else if (img_no == 1) return bn::sprite_items::spr_face_icon_mayo.create_sprite(x, y);
        else if (img_no == 2) return bn::sprite_items::spr_face_icon_riri.create_sprite(x, y);
    }
    else if (img_set == "gallery_thum") {
        if (img_no == 0) return bn::sprite_items::spr_thum_01.create_sprite(x, y);
        else if (img_no == 1) return bn::sprite_items::spr_thum_02.create_sprite(x, y);
        else if (img_no == 2) return bn::sprite_items::spr_thum_03.create_sprite(x, y);
    }
    else if (img_set == "dummy") {
        if (img_no == 0) return bn::sprite_items::spr_dummy.create_sprite(x, y);
    }
    
    return bn::optional<bn::sprite_ptr>();
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
        
        if (rs.visible) {
            rs.sprite = _create_sprite_from_set(s.image_set, s.image_no, rs.x, rs.y);
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

// ==========================================
// 動的UI操作用API
// ==========================================

void UIManager::set_sprite_visible(const bn::string_view& id, bool visible) {
    for (auto& s : sprites_) {
        if (s.id == id) {
            s.visible = visible;
            if (s.visible && s.sprite) {
                s.sprite->set_visible(true);
            } else if (!s.visible && s.sprite) {
                s.sprite->set_visible(false);
            }
            break;
        }
    }
}

void UIManager::set_sprite_position(const bn::string_view& id, bn::fixed x, bn::fixed y) {
    for (auto& s : sprites_) {
        if (s.id == id) {
            s.x = x;
            s.y = y;
            if (s.sprite) {
                s.sprite->set_position(x, y);
            }
            break;
        }
    }
}

void UIManager::set_sprite_image(const bn::string_view& id, const bn::string_view& image_set, int image_no) {
    for (auto& s : sprites_) {
        if (s.id == id) {
            s.sprite.reset();
            s.sprite = _create_sprite_from_set(image_set, image_no, s.x, s.y);
            if (s.sprite && !s.visible) {
                s.sprite->set_visible(false);
            }
            break;
        }
    }
}

void UIManager::set_text(const bn::string_view& id, const bn::string_view& text) {
    for (auto& t : texts_) {
        if (t.id == id) {
            if (t.text != text) {
                t.text = text;
                t.sprites.clear();
                t.dirty = true; // 変更時のみ再生成をトリガー
            }
            break;
        }
    }
}

void UIManager::set_text_visible(const bn::string_view& id, bool visible) {
    for (auto& t : texts_) {
        if (t.id == id) {
            if (t.visible != visible) {
                t.visible = visible;
                if (!t.visible) {
                    t.sprites.clear();
                } else {
                    t.dirty = true; // 表示に戻ったときは再生成が必要
                }
            }
            break;
        }
    }
}
