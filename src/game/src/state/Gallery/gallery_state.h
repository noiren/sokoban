#ifndef GALLERY_STATE_H
#define GALLERY_STATE_H

#include "state/state.h"
#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "gfx/ui_manager.h"

// ギャラリーモード: スチル/立ち絵/BGM鑑賞 (TODO: 実装予定)
class GalleryState : public State {
public:
    GalleryState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void update_browse(StateManager& sm, SharedContext& ctx);

    bn::optional<UIManager> ui_manager_;
    PhaseStep step_;
};

#endif // GALLERY_STATE_H
