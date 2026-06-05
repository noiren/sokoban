#include "gallery_state.h"

#include "state/Manager/state_manager.h"
#include "state/shared_context.h"
#include "input/input_manager.h"
#include "audio/sound_manager.h"
#include "generated/audio_dispatch_bgm.gen.h"
#include "generated/audio_dispatch_se.gen.h"

#include "bn_backdrop.h"
#include "bn_color.h"
#include "bn_music.h"
#include "bn_music_item.h"
#include "bn_sprite_items_japanese_font.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "bn_log.h"

// ============================================================
// 共通ヘルパー
// ============================================================
namespace {

// スチル・立ち絵などカテゴリ一致でフラグ確認してリスト構築
int build_gallery_list_by_cat(const char* category, const SaveSlot& slot, int* out, int max_count) {
    int count = 0;
    for (uint16_t i = 0; i < kGalleryCount && count < max_count; ++i) {
        if (bn::string_view(g_gallery[i].category) != bn::string_view(category)) continue;
        int16_t flag = g_gallery[i].unlock_flag;
        bool unlocked = (flag < 0) || save_slot_get_flag(slot, flag);
        if (unlocked) {
            out[count++] = (int)i;
        }
    }
    return count;
}

void draw_list(bn::sprite_text_generator& gen,
               bn::vector<bn::sprite_ptr, 64>& sprites,
               const char* title,
               const char** names, int count, int cursor,
               const char* hint) {
    gen.set_center_alignment();
    gen.generate(0, -76, title, sprites);

    gen.set_left_alignment();
    const int spacing = 14;
    int y = -48;

    if (count == 0) {
        gen.set_center_alignment();
        gen.generate(0, 0, "(解禁済みなし)", sprites);
    } else {
        int start_i = cursor - 4;
        if (start_i < 0) start_i = 0;
        int end_i = start_i + 8;
        if (end_i > count) {
            end_i = count;
            start_i = end_i - 8;
            if (start_i < 0) start_i = 0;
        }
        for (int i = start_i; i < end_i; ++i) {
            bn::string<48> line;
            line.append(cursor == i ? ">" : " ");
            line.append(names[i]);
            gen.generate(-112, y, line, sprites);
            y += spacing;
        }
    }

    gen.set_center_alignment();
    gen.generate(0, 76, hint, sprites);
}

} // namespace

// ============================================================
// GalleryState (メニュー)
// ============================================================
GalleryState::GalleryState()
    : cursor_(0) {}

void GalleryState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    cursor_ = 0;
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
    constexpr int ITEM_COUNT = 4;
    bool changed = false;

    if (inp.is_repeat(Action::MoveUp)) {
        cursor_--;
        if (cursor_ < 0) cursor_ = ITEM_COUNT - 1;
        changed = true;
        SoundManager::instance().play_move();
    }
    if (inp.is_repeat(Action::MoveDown)) {
        cursor_++;
        if (cursor_ >= ITEM_COUNT) cursor_ = 0;
        changed = true;
        SoundManager::instance().play_move();
    }
    if (inp.is_triggered(Action::Decide)) {
        switch (cursor_) {
            case 0: sm.change_state(StateID::GALLERY_STILL);  return;
            case 1: sm.change_state(StateID::GALLERY_TACHI);  return;
            case 2: sm.change_state(StateID::GALLERY_EVENT);  return;
            case 3: sm.change_state(StateID::GALLERY_AUDIO);  return;
        }
    }
    if (inp.is_triggered(Action::Cancel)) {
        sm.change_state(StateID::MENU);
        return;
    }
    if (changed) redraw(ctx);
}

void GalleryState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    sprites_.clear();
    bn::backdrop::remove_color();
}

void GalleryState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) return;
    sprites_.clear();
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "GALLERY", sprites_);

    ctx.text_generator->set_left_alignment();
    const int spacing = 14;
    int y = -32;
    const char* items[] = {
        "スチル鑑賞",
        "立ち絵鑑賞",
        "イベントセレクト",
        "BGM/SE鑑賞",
    };
    for (int i = 0; i < 4; ++i) {
        bn::string<32> line;
        line.append(cursor_ == i ? ">" : " ");
        line.append(items[i]);
        ctx.text_generator->generate(-80, y, line, sprites_);
        y += spacing;
    }
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 72, "U/D A=open B=menu", sprites_);
}

// ============================================================
// GalleryStillState (スチル鑑賞)
// ============================================================
GalleryStillState::GalleryStillState()
    : item_count_(0), cursor_(0) {}

void GalleryStillState::build_list(const SaveSlot& slot) {
    item_count_ = build_gallery_list_by_cat("still", slot, items_, 64);
    if (cursor_ >= item_count_) cursor_ = 0;
}

void GalleryStillState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    cursor_ = 0;
    if (ctx.save) build_list(ctx.save->slots[ctx.active_slot]);
    bn::backdrop::set_color(bn::color(0, 0, 0));
    sprites_.clear();
    redraw(ctx);
}

void GalleryStillState::update(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    bool changed = false;
    if (item_count_ > 0) {
        if (inp.is_repeat(Action::MoveUp)) {
            cursor_--;
            if (cursor_ < 0) cursor_ = item_count_ - 1;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveDown)) {
            cursor_++;
            if (cursor_ >= item_count_) cursor_ = 0;
            changed = true; SoundManager::instance().play_move();
        }
        // TODO: Aで全画面表示 (スチル画像ローダー実装後)
    }
    if (inp.is_triggered(Action::Cancel)) {
        sm.change_state(StateID::GALLERY);
        return;
    }
    if (changed) redraw(ctx);
}

void GalleryStillState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    sprites_.clear();
    bn::backdrop::remove_color();
}

void GalleryStillState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) return;
    sprites_.clear();
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "スチル鑑賞", sprites_);

    ctx.text_generator->set_left_alignment();
    const int spacing = 14;
    int y = -48;
    if (item_count_ == 0) {
        ctx.text_generator->set_center_alignment();
        ctx.text_generator->generate(0, 0, "(解禁済みなし)", sprites_);
    } else {
        int start_i = cursor_ - 4;
        if (start_i < 0) start_i = 0;
        int end_i = start_i + 8;
        if (end_i > item_count_) { end_i = item_count_; start_i = end_i - 8; if (start_i < 0) start_i = 0; }
        for (int i = start_i; i < end_i; ++i) {
            bn::string<48> line;
            line.append(cursor_ == i ? ">" : " ");
            line.append(g_gallery[items_[i]].ja);
            ctx.text_generator->generate(-112, y, line, sprites_);
            y += spacing;
        }
    }
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 76, "U/D A=view B=back", sprites_);
}

// ============================================================
// GalleryTachiState (立ち絵鑑賞)
// ============================================================
GalleryTachiState::GalleryTachiState()
    : item_count_(0), cursor_(0) {}

void GalleryTachiState::build_list(const SaveSlot& slot) {
    item_count_ = build_gallery_list_by_cat("tachi-e", slot, items_, 64);
    if (cursor_ >= item_count_) cursor_ = 0;
}

void GalleryTachiState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    cursor_ = 0;
    if (ctx.save) build_list(ctx.save->slots[ctx.active_slot]);
    bn::backdrop::set_color(bn::color(0, 0, 0));
    sprites_.clear();
    redraw(ctx);
}

void GalleryTachiState::update(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    bool changed = false;
    if (item_count_ > 0) {
        if (inp.is_repeat(Action::MoveUp)) {
            cursor_--;
            if (cursor_ < 0) cursor_ = item_count_ - 1;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveDown)) {
            cursor_++;
            if (cursor_ >= item_count_) cursor_ = 0;
            changed = true; SoundManager::instance().play_move();
        }
        // TODO: A で立ち絵スプライト表示
    }
    if (inp.is_triggered(Action::Cancel)) {
        sm.change_state(StateID::GALLERY);
        return;
    }
    if (changed) redraw(ctx);
}

void GalleryTachiState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    sprites_.clear();
    bn::backdrop::remove_color();
}

void GalleryTachiState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) return;
    sprites_.clear();
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "立ち絵鑑賞", sprites_);

    ctx.text_generator->set_left_alignment();
    const int spacing = 14;
    int y = -48;
    if (item_count_ == 0) {
        ctx.text_generator->set_center_alignment();
        ctx.text_generator->generate(0, 0, "(解禁済みなし)", sprites_);
    } else {
        int start_i = cursor_ - 4;
        if (start_i < 0) start_i = 0;
        int end_i = start_i + 8;
        if (end_i > item_count_) { end_i = item_count_; start_i = end_i - 8; if (start_i < 0) start_i = 0; }
        for (int i = start_i; i < end_i; ++i) {
            bn::string<48> line;
            line.append(cursor_ == i ? ">" : " ");
            line.append(g_gallery[items_[i]].ja);
            ctx.text_generator->generate(-112, y, line, sprites_);
            y += spacing;
        }
    }
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 76, "U/D A=view B=back", sprites_);
}

// ============================================================
// GalleryEventState (イベントセレクト鑑賞)
// ============================================================
GalleryEventState::GalleryEventState()
    : item_count_(0), cursor_(0) {}

void GalleryEventState::build_list(const SaveSlot& slot) {
    item_count_ = build_gallery_list_by_cat("event", slot, items_, 64);
    if (cursor_ >= item_count_) cursor_ = 0;
}

void GalleryEventState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    cursor_ = 0;
    if (ctx.save) build_list(ctx.save->slots[ctx.active_slot]);
    bn::backdrop::set_color(bn::color(0, 0, 0));
    sprites_.clear();
    redraw(ctx);
}

void GalleryEventState::resume(StateManager& /*sm*/, SharedContext& ctx) {
    // イベント再生から戻ってきたので再構築
    if (ctx.save) build_list(ctx.save->slots[ctx.active_slot]);
    sprites_.clear();
    redraw(ctx);
}

void GalleryEventState::update(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    bool changed = false;
    if (item_count_ > 0) {
        if (inp.is_repeat(Action::MoveUp)) {
            cursor_--;
            if (cursor_ < 0) cursor_ = item_count_ - 1;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveDown)) {
            cursor_++;
            if (cursor_ >= item_count_) cursor_ = 0;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_triggered(Action::Decide)) {
            // resource_id がイベントIDとして使われている
            const char* event_id = g_gallery[items_[cursor_]].resource_id;
            ctx.target_event_id = bn::string_view(event_id);
            // EVT_ / SEVT_ どちらか判定
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
    }
    if (inp.is_triggered(Action::Cancel)) {
        sm.change_state(StateID::GALLERY);
        return;
    }
    if (changed) redraw(ctx);
}

void GalleryEventState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    sprites_.clear();
    bn::backdrop::remove_color();
}

void GalleryEventState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) return;
    sprites_.clear();
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "イベントセレクト", sprites_);

    ctx.text_generator->set_left_alignment();
    const int spacing = 14;
    int y = -48;
    if (item_count_ == 0) {
        ctx.text_generator->set_center_alignment();
        ctx.text_generator->generate(0, 0, "(解禁済みなし)", sprites_);
    } else {
        int start_i = cursor_ - 4;
        if (start_i < 0) start_i = 0;
        int end_i = start_i + 8;
        if (end_i > item_count_) { end_i = item_count_; start_i = end_i - 8; if (start_i < 0) start_i = 0; }
        for (int i = start_i; i < end_i; ++i) {
            bn::string<48> line;
            line.append(cursor_ == i ? ">" : " ");
            line.append(g_gallery[items_[i]].ja);
            ctx.text_generator->generate(-112, y, line, sprites_);
            y += spacing;
        }
    }
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 76, "U/D A=play B=back", sprites_);
}

// ============================================================
// GalleryAudioState (BGM/SE鑑賞)
// ============================================================
GalleryAudioState::GalleryAudioState()
    : audio_mode_(0),
      bgm_count_(0), bgm_cursor_(0),
      se_count_(0),  se_cursor_(0) {}

void GalleryAudioState::build_list(const SaveSlot& slot) {
    bgm_count_ = build_gallery_list_by_cat("bgm", slot, bgm_items_, 32);
    se_count_  = build_gallery_list_by_cat("se",  slot, se_items_,  32);
    if (bgm_cursor_ >= bgm_count_) bgm_cursor_ = 0;
    if (se_cursor_  >= se_count_)  se_cursor_  = 0;
}

void GalleryAudioState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    audio_mode_ = 0; bgm_cursor_ = 0; se_cursor_ = 0;
    if (ctx.save) build_list(ctx.save->slots[ctx.active_slot]);
    bn::backdrop::set_color(bn::color(0, 0, 0));
    sprites_.clear();
    redraw(ctx);
}

void GalleryAudioState::update(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    bool changed = false;

    // L/R でBGM/SE切り替え
    if (inp.is_triggered(Action::LButton) || inp.is_triggered(Action::RButton)) {
        SoundManager::instance().stop_bgm(0);
        audio_mode_ = 1 - audio_mode_;
        changed = true;
    }

    int& cur   = (audio_mode_ == 0) ? bgm_cursor_ : se_cursor_;
    int  count = (audio_mode_ == 0) ? bgm_count_  : se_count_;

    if (count > 0) {
        if (inp.is_repeat(Action::MoveUp)) {
            cur--; if (cur < 0) cur = count - 1;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveDown)) {
            cur++; if (cur >= count) cur = 0;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_triggered(Action::Decide)) {
            if (audio_mode_ == 0) {
                // BGM
                int gi = bgm_items_[bgm_cursor_];
                BgmId id = BgmId::COUNT;
                // resource_id == BgmId名 で検索
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
                // SE
                int gi = se_items_[se_cursor_];
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
            changed = true;
        }
    }
    if (inp.is_triggered(Action::Cancel)) {
        SoundManager::instance().stop_bgm(0);
        sm.change_state(StateID::GALLERY);
        return;
    }
    if (changed) redraw(ctx);
}

void GalleryAudioState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    SoundManager::instance().stop_bgm(0);
    sprites_.clear();
    bn::backdrop::remove_color();
}

void GalleryAudioState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) return;
    sprites_.clear();

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76,
        (audio_mode_ == 0) ? "BGM鑑賞" : "SE鑑賞", sprites_);

    // モード表示
    ctx.text_generator->set_left_alignment();
    bn::string<16> mode_line;
    mode_line.append(audio_mode_ == 0 ? "[BGM] SE " : " BGM [SE]");
    ctx.text_generator->generate(-112, -60, mode_line, sprites_);

    const int spacing = 14;
    int y = -44;

    int  count = (audio_mode_ == 0) ? bgm_count_ : se_count_;
    int* items = (audio_mode_ == 0) ? bgm_items_ : se_items_;
    int& cur   = (audio_mode_ == 0) ? bgm_cursor_ : se_cursor_;

    if (count == 0) {
        ctx.text_generator->set_center_alignment();
        ctx.text_generator->generate(0, 0, "(解禁済みなし)", sprites_);
    } else {
        int start_i = cur - 4;
        if (start_i < 0) start_i = 0;
        int end_i = start_i + 7;
        if (end_i > count) { end_i = count; start_i = end_i - 7; if (start_i < 0) start_i = 0; }
        for (int i = start_i; i < end_i; ++i) {
            bn::string<48> line;
            line.append(cur == i ? ">" : " ");
            line.append(g_gallery[items[i]].ja);
            ctx.text_generator->generate(-112, y, line, sprites_);
            y += spacing;
        }
    }
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 64,  "A=play/stop", sprites_);
    ctx.text_generator->generate(0, 76, "L/R=BGM/SE  B=back", sprites_);
}
