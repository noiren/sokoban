#ifndef GALLERY_STATE_H
#define GALLERY_STATE_H

#include "state/state.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"
#include "bn_sprite_items_japanese_font.h"
#include "generated/generated_fix_data.h"
#include "ui/GenericTextMenu/generic_text_menu.h"

// ==========================================
// ギャラリーメニュー（ハブ画面：背景＋ホットスポット＋吹き出し）
// ==========================================
class GalleryState : public State {
public:
    GalleryState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}
    void resume(StateManager& sm, SharedContext& ctx) override;

private:
    void redraw(SharedContext& ctx);

    int cursor_ = 0;
    bn::optional<bn::regular_bg_ptr> hub_bg_;
    bn::vector<bn::sprite_ptr, 96> sprites_;
};

// ==========================================
// スチル鑑賞
// ==========================================
class GalleryStillState : public State {
public:
    GalleryStillState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}
    void resume(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}

private:
    enum class SubPhase : uint8_t { List, Viewing };

    void redraw(SharedContext& ctx);
    void build_list();
    void rebuild_still_menu(const SharedContext& ctx);

    int items_[64];
    int item_count_;

    SubPhase sub_phase_;
    GenericTextMenu menu_;
    bn::optional<bn::regular_bg_ptr> viewer_bg_;

    bn::vector<bn::sprite_ptr, 160> sprites_;
};

// ==========================================
// 立ち絵鑑賞
// ==========================================
class GalleryTachiState : public State {
public:
    GalleryTachiState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}
    void resume(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}

private:
    enum class SubPhase : uint8_t { List, Viewing };

    void redraw(SharedContext& ctx);
    void build_list();
    void rebuild_tachi_menu(const SharedContext& ctx);

    int items_[64];
    int item_count_;

    SubPhase sub_phase_;
    GenericTextMenu menu_;
    bn::optional<bn::sprite_ptr> viewer_sprite_;

    bn::vector<bn::sprite_ptr, 160> sprites_;
};

// ==========================================
// イベントセレクト鑑賞
// ==========================================
class GalleryEventState : public State {
public:
    GalleryEventState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}
    void resume(StateManager& sm, SharedContext& ctx) override;

private:
    void redraw(SharedContext& ctx);
    void build_list();
    void rebuild_event_menu(const SharedContext& ctx);

    int items_[64];
    int item_count_;

    GenericTextMenu menu_;
    bn::vector<bn::sprite_ptr, 160> sprites_;
};

// ==========================================
// BGM/SE鑑賞
// ==========================================
class GalleryAudioState : public State {
public:
    GalleryAudioState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}
    void resume(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}

private:
    void redraw(SharedContext& ctx);
    void build_list();
    void rebuild_audio_menu(const SharedContext& ctx);

    // 0=BGM mode, 1=SE mode
    int audio_mode_;

    int bgm_items_[32];
    int bgm_count_;
    int bgm_cursor_;

    int se_items_[32];
    int se_count_;
    int se_cursor_;

    GenericTextMenu menu_;
    bn::vector<bn::sprite_ptr, 160> sprites_;
};

#endif // GALLERY_STATE_H
