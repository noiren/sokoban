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

void UIManager::clear_bg() {
    bg_.reset();
}

void UIManager::clear_all() {
    clear_bg();
    
    for (UIImage* img : images_) image_pool_.destroy(*img);
    for (UIText* txt : texts_) text_pool_.destroy(*txt);
    for (UIAnim* anim : anims_) anim_pool_.destroy(*anim);
    
    images_.clear();
    texts_.clear();
    anims_.clear();
    nodes_.clear();
}

void UIManager::load_screen(const ui_types::ScreenData& screen_data) {
    clear_all();

    _set_bg_from_string(screen_data.bg_image_id);

    // 画像ノード生成
    for(int i = 0; i < screen_data.sprite_count; ++i) {
        const auto& s = screen_data.sprites[i];
        UIImage& img = image_pool_.create(s.id, bn::fixed(s.x), bn::fixed(s.y), bn::fixed(s.rotation), s.visible);
        if (s.visible) {
            img.set_sprite(_create_sprite_from_set(s.image_set, s.image_no, img.get_x(), img.get_y()));
        }
        nodes_.push_back(&img);
        images_.push_back(&img);
    }

    // テキストノード生成
    for(int i = 0; i < screen_data.text_count; ++i) {
        const auto& t = screen_data.texts[i];
        UIText& txt = text_pool_.create(t.id, bn::fixed(t.x), bn::fixed(t.y), t.visible,
                                        t.align, t.font_size,
                                        t.blink, t.blink_interval, text_gen_);
        txt.set_text(t.text);
        nodes_.push_back(&txt);
        texts_.push_back(&txt);
    }

    // ★アニメーションノード生成
    for(int i = 0; i < screen_data.anim_entry_count; ++i) {
        const auto& entry = screen_data.anim_entries[i];
        UIAnim& anim = anim_pool_.create(entry.id);

        // プリセット紐付け
        for(int p = 0; p < screen_data.anim_preset_count; ++p) {
            if (bn::string_view(screen_data.anim_presets[p].id) == entry.preset_id) {
                anim.set_preset(&screen_data.anim_presets[p]);
                break;
            }
        }

        // ターゲット紐付け
        for(int t = 0; t < entry.target_count; ++t) {
            bn::string_view target_id = entry.target_ids[t];
            if (UIImage* img = get_image(target_id)) anim.add_target(img);
            else if (UIText* txt = get_text(target_id)) anim.add_target(txt);
        }

        nodes_.push_back(&anim);
        anims_.push_back(&anim);
        
        anim.play(); // ロード直後に自動再生を開始する
    }
}

void UIManager::update() {
    for (UINode* node : nodes_) {
        node->update();
    }
}

UIImage* UIManager::get_image(const bn::string_view& id) {
    for(UIImage* img : images_) { if (img->get_id() == id) return img; }
    return nullptr;
}

UIText* UIManager::get_text(const bn::string_view& id) {
    for(UIText* txt : texts_) { if (txt->get_id() == id) return txt; }
    return nullptr;
}

UIAnim* UIManager::get_anim(const bn::string_view& id) {
    for(UIAnim* anim : anims_) { if (anim->get_id() == id) return anim; }
    return nullptr;
}

void UIManager::change_sprite_image(UIImage* node, const bn::string_view& image_set, int image_no) {
    if (!node) return;
    node->set_sprite(_create_sprite_from_set(image_set, image_no, node->get_x(), node->get_y()));
}

void UIManager::change_sprite_image_by_id(UIImage* node, const bn::string_view& image_id) {
    if (!node) return;
    // chara_portraits セット内の image_id -> image_no マッピング
    // 順序は assets_list.json の chara_portraits.items に対応:
    //   0: spr_ch_mayo_normal
    //   1: spr_ch_mayo_smile
    //   2: spr_ch_mayo_sad
    //   3: spr_ch_mayo_angry
    //   4: spr_ch_mayo_fun
    //   5: spr_ch_riri_normal (chara_b 用素材が揃うまでの代替)
    
    if      (image_id == "mayo_normal")      change_sprite_image(node, "chara_portraits", 0);
    else if (image_id == "chara_a_normal")   change_sprite_image(node, "chara_portraits", 0); // Alias
    else if (image_id == "mayo_smile")       change_sprite_image(node, "chara_portraits", 1);
    else if (image_id == "mayo_sad")         change_sprite_image(node, "chara_portraits", 2);
    else if (image_id == "mayo_angry")       change_sprite_image(node, "chara_portraits", 3);
    else if (image_id == "mayo_fun")         change_sprite_image(node, "chara_portraits", 4);
    else if (image_id == "chara_b_normal" ||
             image_id == "chara_b_smile" ||
             image_id == "chara_b_sad")      change_sprite_image(node, "chara_portraits", 5);
    // image_id が不明な場合は何もしない (既存のスプライトを維持)
}

void UIManager::_set_bg_from_string(bn::string_view bg_id) {
    if (bg_id == "stl_logo") bg_ = bn::regular_bg_items::stl_logo.create_bg(8, 48);
    else if (bg_id == "stl_title") bg_ = bn::regular_bg_items::stl_title.create_bg(8, 48);
    else if (bg_id == "stl_attention") bg_ = bn::regular_bg_items::stl_attention.create_bg(8, 48);
    else if (bg_id == "still_mainmenu") bg_ = bn::regular_bg_items::still_mainmenu.create_bg(8, 48);
    else if (bg_id == "still_practice") bg_ = bn::regular_bg_items::still_practice.create_bg(8, 48);
    else if (bg_id == "still_sokoban_main") bg_ = bn::regular_bg_items::still_sokoban_main.create_bg(8, 48);
    else if (bg_id == "still_event") bg_ = bn::regular_bg_items::still_event.create_bg(8, 48);
    else if (bg_id == "gba_event") bg_ = bn::regular_bg_items::gba_event.create_bg(8, 48);
    else if (bg_id == "still_gallerymenu") bg_ = bn::regular_bg_items::still_gallerymenu.create_bg(8, 48);
    else if (bg_id == "still_bgm") bg_ = bn::regular_bg_items::still_bgm.create_bg(8, 48);
    else if (bg_id == "still_se") bg_ = bn::regular_bg_items::still_se.create_bg(8, 48);
    else if (bg_id == "still_viewbustup") bg_ = bn::regular_bg_items::still_viewbustup.create_bg(8, 48);
    else if (bg_id == "still_viewstill") bg_ = bn::regular_bg_items::still_viewstill.create_bg(8, 48);
    else if (bg_id == "still_saveattention") bg_ = bn::regular_bg_items::still_saveattention.create_bg(8, 48);
    else clear_bg();
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