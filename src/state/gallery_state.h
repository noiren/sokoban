#ifndef GALLERY_STATE_H
#define GALLERY_STATE_H

#include "state/state.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

// ギャラリーモード: スチル/立ち絵/BGM鑑賞 (TODO: 実装予定)
class GalleryState : public State {
public:
    explicit GalleryState(bn::sprite_text_generator& text_gen);
    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

private:
    bn::sprite_text_generator& text_gen_;
    bn::vector<bn::sprite_ptr, 32> sprites_;
};

#endif // GALLERY_STATE_H
