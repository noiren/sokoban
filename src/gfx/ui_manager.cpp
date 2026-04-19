#include "ui_manager.h"

// Butano自動生成ヘッダ
#include "bn_regular_bg_items_bg_logo.h"
#include "bn_regular_bg_items_bg_title.h"

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

    // テキスト要素のセット
    for(int i = 0; i < screen_data.text_count; ++i) {
        const auto& t = screen_data.texts[i];
        RuntimeUIText rt;
        rt.id = t.id;
        rt.x = t.x;
        rt.y = t.y;
        rt.blink = t.blink;
        rt.blink_interval = t.blink_interval;
        rt.visible = t.visible;
        rt.text = t.text;
        
        texts_.push_back(rt);
    }
}
