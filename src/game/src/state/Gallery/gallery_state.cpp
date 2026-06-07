#include "gallery_state.h"

#include "state/Manager/state_manager.h"
#include "state/state_id.h"
#include "state/shared_context.h"
#include "input/input_manager.h"
#include "audio/sound_manager.h"
#include "generated/audio_dispatch_bgm.gen.h"
#include "generated/audio_dispatch_se.gen.h"
#include "generated/audio_ids.h"
#include "generated/chara_portraits.gen.h"

#include "bn_backdrop.h"
#include "bn_color.h"
#include "bn_music.h"
#include "bn_music_item.h"
#include "bn_sprite_items_japanese_font.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"

#include "bn_assert.h"

#include "save/save_data.h"

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
#include "bn_sprite_items_spr_icon_clapper.h"
#include "bn_sprite_items_spr_icon_note.h"
#include "bn_sprite_items_spr_paper_small.h"
#include "bn_sprite_items_spr_selector_gold_bl.h"
#include "bn_sprite_items_spr_selector_gold_br.h"
#include "bn_sprite_items_spr_selector_gold_tl.h"
#include "bn_sprite_items_spr_selector_gold_tr.h"
#include "bn_sprite_items_spr_sparkle.h"
#include "bn_sprite_items_spr_thum_01.h"
#include "bn_sprite_items_spr_thum_02.h"

// ============================================================
// 共通ヘルパー
// ============================================================
namespace {

// 解禁済みなら true（unlock_flag<0 は常時解禁）。debug_gallery_unlock_all でデバッグ上書き。
bool gallery_entry_unlocked(const SaveSlot* slot, int gi, bool debug_gallery_all) {
    if (debug_gallery_all) {
        return true;
    }
    const int16_t flag = g_gallery[gi].unlock_flag;
    if (flag < 0) {
        return true;
    }
    if (!slot) {
        return false;
    }
    return save_slot_get_flag(*slot, flag);
}

bn::string_view gallery_row_label(const SaveSlot* slot, int gi, bool debug_gallery_all) {
    if (gallery_entry_unlocked(slot, gi, debug_gallery_all)) {
        return bn::string_view(g_gallery[gi].ja);
    }
    return "？？？？？";
}

int build_gallery_list_by_cat(const char* category, int* out, int max_count) {
    int count = 0;
    for (uint16_t i = 0; i < kGalleryCount && count < max_count; ++i) {
        if (bn::string_view(g_gallery[i].category) != bn::string_view(category)) {
            continue;
        }
        out[count++] = static_cast<int>(i);
    }
    return count;
}

// UIManager::change_sprite_image_by_id と同じ正規化（create_by_id 用）
bn::string_view gallery_resolve_portrait_id(bn::string_view id) {
    if (id == "mayo_normal" || id == "chara_a_normal") {
        return "mayo_normal_1";
    }
    if (id == "mayo_smile") {
        return "mayo_smile_1";
    }
    if (id == "mayo_sad") {
        return "mayo_sad_1";
    }
    if (id == "mayo_angry") {
        return "mayo_angry_1";
    }
    if (id == "mayo_fun") {
        return "mayo_happy_1";
    }
    if (id == "mayo_surprised") {
        return "mayo_surprised_1";
    }
    if (id == "mayo_happy1") {
        return "mayo_happy_1";
    }
    if (id == "mayo_happy2") {
        return "mayo_happy_2";
    }
    if (id == "mayo_happy3") {
        return "mayo_happy_3";
    }
    if (id == "chara_b_normal" || id == "chara_b_smile" ||
        id == "chara_b_happy1" || id == "chara_b_sad") {
        return "riri_normal";
    }
    return id;
}

bn::optional<bn::regular_bg_ptr> create_gallery_still_bg(bn::string_view bg_id) {
    if (bg_id == "stl_logo") {
        return bn::regular_bg_items::stl_logo.create_bg(8, 48);
    }
    if (bg_id == "stl_title") {
        return bn::regular_bg_items::stl_title.create_bg(8, 48);
    }
    if (bg_id == "stl_attention") {
        return bn::regular_bg_items::stl_attention.create_bg(8, 48);
    }
    if (bg_id == "still_mainmenu") {
        return bn::regular_bg_items::still_mainmenu.create_bg(8, 48);
    }
    if (bg_id == "still_practice") {
        return bn::regular_bg_items::still_practice.create_bg(8, 48);
    }
    if (bg_id == "still_sokoban_main") {
        return bn::regular_bg_items::still_sokoban_main.create_bg(8, 48);
    }
    if (bg_id == "still_event") {
        return bn::regular_bg_items::still_event.create_bg(8, 48);
    }
    if (bg_id == "gba_event") {
        return bn::regular_bg_items::gba_event.create_bg(8, 48);
    }
    if (bg_id == "still_gallerymenu") {
        return bn::regular_bg_items::still_gallerymenu.create_bg(8, 48);
    }
    if (bg_id == "still_bgm") {
        return bn::regular_bg_items::still_bgm.create_bg(8, 48);
    }
    if (bg_id == "still_se") {
        return bn::regular_bg_items::still_se.create_bg(8, 48);
    }
    if (bg_id == "still_viewbustup") {
        return bn::regular_bg_items::still_viewbustup.create_bg(8, 48);
    }
    if (bg_id == "still_viewstill") {
        return bn::regular_bg_items::still_viewstill.create_bg(8, 48);
    }
    if (bg_id == "still_saveattention") {
        return bn::regular_bg_items::still_saveattention.create_bg(8, 48);
    }
    return bn::optional<bn::regular_bg_ptr>();
}

// ギャラリーハブ：still_gallerymenu 本番レイアウトに合わせたホットスポット（画面中央原点）。
// 0=本=スチル / 1=鏡=立ち絵 / 2=映写機=イベント / 3=蓄音機=BGM・SE（画像差し替え後も座標は要調整）
struct GalleryHubSlot {
    bn::fixed hx;
    bn::fixed hy;
    StateID next_state;
    const char* label;
};

constexpr GalleryHubSlot k_gallery_hub_slots[] = {
    { -82, 54, StateID::GALLERY_STILL, "スチル鑑賞" },
    { 58, -20, StateID::GALLERY_TACHI, "立ち絵鑑賞" },
    { -6, 4, StateID::GALLERY_EVENT, "イベント再生" },
    { 90, 22, StateID::GALLERY_AUDIO, "BGM/SE" },
};

constexpr int k_gallery_hub_slot_count = 4;

} // namespace

// ============================================================
// GalleryState（ギャラリーハブ：背景＋ホットスポット＋吹き出し）
// ============================================================
GalleryState::GalleryState() = default;

void GalleryState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    cursor_ = 0;
    hub_bg_.reset();
    hub_bg_ = bn::regular_bg_items::still_gallerymenu.create_bg(8, 48);
    if (hub_bg_) {
        hub_bg_->set_priority(0);
    }
    bn::backdrop::set_color(bn::color(0, 0, 0));
    if (ctx.text_generator) {
        ctx.text_generator->set_palette_item(bn::sprite_items::japanese_font.palette_item());
    }
    sprites_.clear();
    redraw(ctx);
}

void GalleryState::resume(StateManager& /*sm*/, SharedContext& ctx) {
    sprites_.clear();
    redraw(ctx);
}

void GalleryState::update(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();

    if (inp.is_triggered(Action::Cancel)) {
        sm.change_state(StateID::MENU);
        return;
    }

    bool moved = false;
    if (inp.is_repeat(Action::MoveUp)) {
        --cursor_;
        if (cursor_ < 0) {
            cursor_ = k_gallery_hub_slot_count - 1;
        }
        moved = true;
        SoundManager::instance().play_move();
    }
    if (inp.is_repeat(Action::MoveDown)) {
        ++cursor_;
        if (cursor_ >= k_gallery_hub_slot_count) {
            cursor_ = 0;
        }
        moved = true;
        SoundManager::instance().play_move();
    }

    if (inp.is_triggered(Action::Decide)) {
        BN_ASSERT(cursor_ >= 0 && cursor_ < k_gallery_hub_slot_count, "gallery hub cursor");
        sm.change_state(k_gallery_hub_slots[cursor_].next_state);
        return;
    }

    if (moved) {
        redraw(ctx);
    }
}

void GalleryState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    hub_bg_.reset();
    sprites_.clear();
    bn::backdrop::remove_color();
}

void GalleryState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) {
        return;
    }
    sprites_.clear();
    BN_ASSERT(cursor_ >= 0 && cursor_ < k_gallery_hub_slot_count, "gallery hub redraw cursor");

    const GalleryHubSlot& sel = k_gallery_hub_slots[cursor_];

    for (int i = 0; i < k_gallery_hub_slot_count; ++i) {
        const bn::fixed x = k_gallery_hub_slots[i].hx;
        const bn::fixed y = k_gallery_hub_slots[i].hy;
        switch (i) {
            case 0:
                // 本（スチル）— 差し替えまでサムネで目印
                sprites_.push_back(bn::sprite_items::spr_thum_01.create_sprite(x, y));
                break;
            case 1:
                // 鏡（立ち絵）
                sprites_.push_back(bn::sprite_items::spr_thum_02.create_sprite(x, y));
                break;
            case 2:
                // 映写機（イベント）
                sprites_.push_back(bn::sprite_items::spr_icon_clapper.create_sprite(x, y));
                break;
            default:
                // 蓄音機（BGM/SE）
                sprites_.push_back(bn::sprite_items::spr_icon_note.create_sprite(x, y));
                break;
        }
    }

    constexpr bn::fixed k_frame_half_w = 26;
    constexpr bn::fixed k_frame_half_h = 18;
    const bn::fixed box_left = sel.hx - k_frame_half_w;
    const bn::fixed box_right = sel.hx + k_frame_half_w;
    const bn::fixed fy_top = sel.hy - k_frame_half_h;
    const bn::fixed fy_bot = sel.hy + k_frame_half_h;
    sprites_.push_back(bn::sprite_items::spr_selector_gold_tl.create_sprite(box_left, fy_top));
    sprites_.push_back(bn::sprite_items::spr_selector_gold_tr.create_sprite(box_right, fy_top));
    sprites_.push_back(bn::sprite_items::spr_selector_gold_bl.create_sprite(box_left, fy_bot));
    sprites_.push_back(bn::sprite_items::spr_selector_gold_br.create_sprite(box_right, fy_bot));
    sprites_.push_back(bn::sprite_items::spr_sparkle.create_sprite(sel.hx + 22, sel.hy - 12));
    sprites_.push_back(bn::sprite_items::spr_sparkle.create_sprite(sel.hx - 22, sel.hy + 10));

    const bn::fixed bubble_x = sel.hx;
    const bn::fixed bubble_y = sel.hy - 34;
    sprites_.push_back(bn::sprite_items::spr_paper_small.create_sprite(bubble_x, bubble_y));

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -74, "GALLERY", sprites_);
    ctx.text_generator->generate(bubble_x, bubble_y - 2, sel.label, sprites_);
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 72, "U/D=移動 A=決定 B=戻る", sprites_);
}

// ============================================================
// GalleryStillState (スチル鑑賞)
// ============================================================
GalleryStillState::GalleryStillState()
    : item_count_(0), sub_phase_(SubPhase::List) {}

void GalleryStillState::build_list() {
    item_count_ = build_gallery_list_by_cat("still", items_, 64);
}

void GalleryStillState::rebuild_still_menu(const SharedContext& ctx) {
    menu_.clear_items();
    menu_.configure(-112, -48, 14, 8);
    const SaveSlot* slot = (ctx.save) ? &ctx.save->slots[ctx.active_slot] : nullptr;
    for (int i = 0; i < item_count_; ++i) {
        const int gi = items_[i];
        menu_.push_item(gi, gallery_row_label(slot, gi, ctx.debug_gallery_unlock_all));
    }
    menu_.set_cursor(0);
}

void GalleryStillState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    sub_phase_ = SubPhase::List;
    viewer_bg_.reset();
    build_list();
    rebuild_still_menu(ctx);
    bn::backdrop::set_color(bn::color(0, 0, 0));
    if (ctx.text_generator) {
        ctx.text_generator->set_palette_item(bn::sprite_items::japanese_font.palette_item());
    }
    sprites_.clear();
    redraw(ctx);
}

void GalleryStillState::update(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();

    if (sub_phase_ == SubPhase::Viewing) {
        if (inp.is_triggered(Action::Cancel)) {
            viewer_bg_.reset();
            sub_phase_ = SubPhase::List;
            redraw(ctx);
        }
        return;
    }

    int value = 0;
    const auto r = menu_.poll(&value);

    if (r == GenericTextMenu::Poll::Cancelled) {
        sm.change_state(StateID::GALLERY);
        return;
    }

    if (r == GenericTextMenu::Poll::Confirmed && item_count_ > 0) {
        const int gi = value;
        const SaveSlot* slot = ctx.save ? &ctx.save->slots[ctx.active_slot] : nullptr;
        if (!gallery_entry_unlocked(slot, gi, ctx.debug_gallery_unlock_all)) {
            SoundManager::instance().play_se(SeId::Default_Reset);
            return;
        }
        bn::string_view rid(g_gallery[gi].resource_id);
        bn::optional<bn::regular_bg_ptr> bg = create_gallery_still_bg(rid);
        if (bg) {
            bg->set_priority(0);
            viewer_bg_ = bn::move(bg);
            sub_phase_ = SubPhase::Viewing;
            redraw(ctx);
        } else {
            SoundManager::instance().play_se(SeId::Default_Reset);
        }
        return;
    }

    if (r == GenericTextMenu::Poll::Moved) {
        redraw(ctx);
    }
}

void GalleryStillState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    viewer_bg_.reset();
    sprites_.clear();
    bn::backdrop::remove_color();
}

void GalleryStillState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) {
        return;
    }
    sprites_.clear();
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "スチル鑑賞", sprites_);

    if (sub_phase_ == SubPhase::Viewing) {
        ctx.text_generator->set_center_alignment();
        ctx.text_generator->generate(0, 72, "B=一覧に戻る", sprites_);
        return;
    }

    menu_.draw(*ctx.text_generator, sprites_);
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 72, "U/D A=表示 B=戻る", sprites_);
}

// ============================================================
// GalleryTachiState (立ち絵鑑賞)
// ============================================================
GalleryTachiState::GalleryTachiState()
    : item_count_(0), sub_phase_(SubPhase::List) {}

void GalleryTachiState::build_list() {
    item_count_ = build_gallery_list_by_cat("tachi-e", items_, 64);
}

void GalleryTachiState::rebuild_tachi_menu(const SharedContext& ctx) {
    menu_.clear_items();
    menu_.configure(-112, -48, 14, 8);
    const SaveSlot* slot = (ctx.save) ? &ctx.save->slots[ctx.active_slot] : nullptr;
    for (int i = 0; i < item_count_; ++i) {
        const int gi = items_[i];
        menu_.push_item(gi, gallery_row_label(slot, gi, ctx.debug_gallery_unlock_all));
    }
    menu_.set_cursor(0);
}

void GalleryTachiState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    sub_phase_ = SubPhase::List;
    viewer_sprite_.reset();
    build_list();
    rebuild_tachi_menu(ctx);
    bn::backdrop::set_color(bn::color(0, 0, 0));
    if (ctx.text_generator) {
        ctx.text_generator->set_palette_item(bn::sprite_items::japanese_font.palette_item());
    }
    sprites_.clear();
    redraw(ctx);
}

void GalleryTachiState::update(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();

    if (sub_phase_ == SubPhase::Viewing) {
        if (inp.is_triggered(Action::Cancel)) {
            viewer_sprite_.reset();
            sub_phase_ = SubPhase::List;
            redraw(ctx);
        }
        return;
    }

    int value = 0;
    const auto r = menu_.poll(&value);

    if (r == GenericTextMenu::Poll::Cancelled) {
        sm.change_state(StateID::GALLERY);
        return;
    }

    if (r == GenericTextMenu::Poll::Confirmed && item_count_ > 0) {
        const int gi = value;
        const SaveSlot* slot = ctx.save ? &ctx.save->slots[ctx.active_slot] : nullptr;
        if (!gallery_entry_unlocked(slot, gi, ctx.debug_gallery_unlock_all)) {
            SoundManager::instance().play_se(SeId::Default_Reset);
            return;
        }
        bn::string_view rid(g_gallery[gi].resource_id);
        bn::string_view canon = gallery_resolve_portrait_id(rid);
        bn::optional<bn::sprite_ptr> spr = chara_portraits::create_by_id(canon, 0, -8);
        if (spr) {
            viewer_sprite_ = bn::move(spr);
            sub_phase_ = SubPhase::Viewing;
            redraw(ctx);
        } else {
            SoundManager::instance().play_se(SeId::Default_Reset);
        }
        return;
    }

    if (r == GenericTextMenu::Poll::Moved) {
        redraw(ctx);
    }
}

void GalleryTachiState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    viewer_sprite_.reset();
    sprites_.clear();
    bn::backdrop::remove_color();
}

void GalleryTachiState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) {
        return;
    }
    sprites_.clear();
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "立ち絵鑑賞", sprites_);

    if (sub_phase_ == SubPhase::Viewing) {
        ctx.text_generator->set_center_alignment();
        ctx.text_generator->generate(0, 72, "B=一覧に戻る", sprites_);
        return;
    }

    menu_.draw(*ctx.text_generator, sprites_);
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 72, "U/D A=表示 B=戻る", sprites_);
}

// ============================================================
// GalleryEventState (イベントセレクト鑑賞)
// ============================================================
GalleryEventState::GalleryEventState()
    : item_count_(0) {}

void GalleryEventState::build_list() {
    item_count_ = build_gallery_list_by_cat("event", items_, 64);
}

void GalleryEventState::rebuild_event_menu(const SharedContext& ctx) {
    menu_.clear_items();
    menu_.configure(-112, -48, 14, 8);
    const SaveSlot* slot = (ctx.save) ? &ctx.save->slots[ctx.active_slot] : nullptr;
    for (int i = 0; i < item_count_; ++i) {
        const int gi = items_[i];
        menu_.push_item(gi, gallery_row_label(slot, gi, ctx.debug_gallery_unlock_all));
    }
    menu_.set_cursor(0);
}

void GalleryEventState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    build_list();
    rebuild_event_menu(ctx);
    bn::backdrop::set_color(bn::color(0, 0, 0));
    if (ctx.text_generator) {
        ctx.text_generator->set_palette_item(bn::sprite_items::japanese_font.palette_item());
    }
    sprites_.clear();
    redraw(ctx);
}

void GalleryEventState::resume(StateManager& /*sm*/, SharedContext& ctx) {
    build_list();
    rebuild_event_menu(ctx);
    sprites_.clear();
    redraw(ctx);
}

void GalleryEventState::update(StateManager& sm, SharedContext& ctx) {
    int value = 0;
    const auto r = menu_.poll(&value);

    if (r == GenericTextMenu::Poll::Cancelled) {
        sm.change_state(StateID::GALLERY);
        return;
    }

    if (r == GenericTextMenu::Poll::Confirmed && item_count_ > 0) {
        const int gi = value;
        const SaveSlot* slot = ctx.save ? &ctx.save->slots[ctx.active_slot] : nullptr;
        if (!gallery_entry_unlocked(slot, gi, ctx.debug_gallery_unlock_all)) {
            SoundManager::instance().play_se(SeId::Default_Reset);
            return;
        }
        const char* event_id = g_gallery[gi].resource_id;
        ctx.target_event_id = bn::string_view(event_id);
        bn::string_view id_sv(event_id);
        if (id_sv.size() >= 5 &&
            id_sv[0] == 's' && id_sv[1] == 'e' && id_sv[2] == 'v' && id_sv[3] == 't' && id_sv[4] == '_') {
            ctx.event_return_state = StateID::GALLERY_EVENT;
            sm.push_state(StateID::STILL_EVENT);
        } else {
            ctx.event_return_state = StateID::GALLERY_EVENT;
            sm.push_state(StateID::EVENT);
        }
        return;
    }

    if (r == GenericTextMenu::Poll::Moved) {
        redraw(ctx);
    }
}

void GalleryEventState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    sprites_.clear();
    bn::backdrop::remove_color();
}

void GalleryEventState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) {
        return;
    }
    sprites_.clear();
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "イベントセレクト", sprites_);

    menu_.draw(*ctx.text_generator, sprites_);
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 72, "U/D A=再生 B=戻る", sprites_);
}

// ============================================================
// GalleryAudioState (BGM/SE鑑賞)
// ============================================================
GalleryAudioState::GalleryAudioState()
    : audio_mode_(0),
      bgm_count_(0), bgm_cursor_(0),
      se_count_(0), se_cursor_(0) {}

void GalleryAudioState::build_list() {
    bgm_count_ = build_gallery_list_by_cat("bgm", bgm_items_, 32);
    se_count_ = build_gallery_list_by_cat("se", se_items_, 32);
    if (bgm_cursor_ >= bgm_count_) {
        bgm_cursor_ = 0;
    }
    if (se_cursor_ >= se_count_) {
        se_cursor_ = 0;
    }
}

void GalleryAudioState::rebuild_audio_menu(const SharedContext& ctx) {
    menu_.clear_items();
    menu_.configure(-112, -44, 14, 7);
    const SaveSlot* slot = (ctx.save) ? &ctx.save->slots[ctx.active_slot] : nullptr;
    if (audio_mode_ == 0) {
        for (int i = 0; i < bgm_count_; ++i) {
            const int gi = bgm_items_[i];
            menu_.push_item(gi, gallery_row_label(slot, gi, ctx.debug_gallery_unlock_all));
        }
        menu_.set_cursor(bgm_cursor_);
    } else {
        for (int i = 0; i < se_count_; ++i) {
            const int gi = se_items_[i];
            menu_.push_item(gi, gallery_row_label(slot, gi, ctx.debug_gallery_unlock_all));
        }
        menu_.set_cursor(se_cursor_);
    }
}

void GalleryAudioState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    audio_mode_ = 0;
    bgm_cursor_ = 0;
    se_cursor_ = 0;
    build_list();
    rebuild_audio_menu(ctx);
    bn::backdrop::set_color(bn::color(0, 0, 0));
    if (ctx.text_generator) {
        ctx.text_generator->set_palette_item(bn::sprite_items::japanese_font.palette_item());
    }
    sprites_.clear();
    redraw(ctx);
}

void GalleryAudioState::update(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();

    if (inp.is_triggered(Action::LButton) || inp.is_triggered(Action::RButton)) {
        SoundManager::instance().stop_bgm(0);
        if (audio_mode_ == 0) {
            bgm_cursor_ = menu_.cursor_index();
        } else {
            se_cursor_ = menu_.cursor_index();
        }
        audio_mode_ = 1 - audio_mode_;
        rebuild_audio_menu(ctx);
        redraw(ctx);
        return;
    }

    int value = 0;
    const auto r = menu_.poll(&value);

    if (r == GenericTextMenu::Poll::Cancelled) {
        SoundManager::instance().stop_bgm(0);
        sm.change_state(StateID::GALLERY);
        return;
    }

    if (audio_mode_ == 0) {
        bgm_cursor_ = menu_.cursor_index();
    } else {
        se_cursor_ = menu_.cursor_index();
    }

    if (r == GenericTextMenu::Poll::Confirmed) {
        const int gi = value;
        const SaveSlot* slot = ctx.save ? &ctx.save->slots[ctx.active_slot] : nullptr;
        if (!gallery_entry_unlocked(slot, gi, ctx.debug_gallery_unlock_all)) {
            SoundManager::instance().play_se(SeId::Default_Reset);
            redraw(ctx);
            return;
        }
        if (audio_mode_ == 0) {
            BgmId id = BgmId::COUNT;
            for (int b = 0; b < static_cast<int>(BgmId::COUNT); ++b) {
                if (bn::string_view(audio_dispatch::bgm_name(static_cast<BgmId>(b))) ==
                    bn::string_view(g_gallery[gi].resource_id)) {
                    id = static_cast<BgmId>(b);
                    break;
                }
            }
            if (id != BgmId::COUNT) {
                SoundManager::instance().play_bgm(id, true, 0, true);
            }
        } else {
            SeId id = SeId::COUNT;
            for (int s = 0; s < static_cast<int>(SeId::COUNT); ++s) {
                if (bn::string_view(audio_dispatch::se_name(static_cast<SeId>(s))) ==
                    bn::string_view(g_gallery[gi].resource_id)) {
                    id = static_cast<SeId>(s);
                    break;
                }
            }
            if (id != SeId::COUNT) {
                SoundManager::instance().play_se(id);
            }
        }
        redraw(ctx);
        return;
    }

    if (r == GenericTextMenu::Poll::Moved) {
        redraw(ctx);
    }
}

void GalleryAudioState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    SoundManager::instance().stop_bgm(0);
    sprites_.clear();
    bn::backdrop::remove_color();
}

void GalleryAudioState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) {
        return;
    }
    sprites_.clear();

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76,
        (audio_mode_ == 0) ? "BGM鑑賞" : "SE鑑賞", sprites_);

    ctx.text_generator->set_left_alignment();
    bn::string<16> mode_line;
    mode_line.append(audio_mode_ == 0 ? "[BGM] SE " : " BGM [SE]");
    ctx.text_generator->generate(-112, -60, mode_line, sprites_);

    menu_.draw(*ctx.text_generator, sprites_);

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 64, "A=play", sprites_);
    ctx.text_generator->generate(0, 76, "L/R=BGM/SE  B=back", sprites_);
}
