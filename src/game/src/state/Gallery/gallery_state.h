#ifndef GALLERY_STATE_H
#define GALLERY_STATE_H

#include "state/state.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_items_japanese_font.h"
#include "save/save_data.h"
#include "save/game_flags.h"
#include "generated/generated_fix_data.h"

// ==========================================
// ギャラリーメニュー
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

    int cursor_;
    bn::vector<bn::sprite_ptr, 64> sprites_;
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
    void redraw(SharedContext& ctx);
    void build_list(const SaveSlot& slot);

    // 表示可能なスチルのインデックス (g_gallery)
    int items_[64];
    int item_count_;
    int cursor_;
    bn::vector<bn::sprite_ptr, 64> sprites_;
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
    void redraw(SharedContext& ctx);
    void build_list(const SaveSlot& slot);

    int items_[64];
    int item_count_;
    int cursor_;
    bn::vector<bn::sprite_ptr, 64> sprites_;
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
    void build_list(const SaveSlot& slot);

    int items_[64];
    int item_count_;
    int cursor_;
    bn::vector<bn::sprite_ptr, 64> sprites_;
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
    void build_list(const SaveSlot& slot);

    // 0=BGM mode, 1=SE mode
    int audio_mode_;

    int bgm_items_[32];
    int bgm_count_;
    int bgm_cursor_;

    int se_items_[32];
    int se_count_;
    int se_cursor_;

    bn::vector<bn::sprite_ptr, 64> sprites_;
};

#endif // GALLERY_STATE_H
