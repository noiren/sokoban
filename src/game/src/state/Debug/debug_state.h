#ifndef DEBUG_STATE_H
#define DEBUG_STATE_H

#include "state/state.h"

#include "generated/audio_ids.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_fixed.h"
#include "bn_vector.h"

enum class DebugScreen {
    Root,
    BgmList,
    BgmTest,
    SeList,
    SeTest,
    EffectTest, // 追加
};

class DebugState : public State {
public:
    DebugState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void redraw(SharedContext& ctx);

    void update_root(StateManager& sm, SharedContext& ctx);
    void draw_root(SharedContext& ctx);

    void update_bgm_list(StateManager& sm, SharedContext& ctx);
    void draw_bgm_list(SharedContext& ctx);

    void update_bgm_test(StateManager& sm, SharedContext& ctx);
    void draw_bgm_test(SharedContext& ctx);

    void update_se_list(StateManager& sm, SharedContext& ctx);
    void draw_se_list(SharedContext& ctx);

    void update_se_test(StateManager& sm, SharedContext& ctx);
    void draw_se_test(SharedContext& ctx);

    void update_effect_test(StateManager& sm, SharedContext& ctx);
    void draw_effect_test(SharedContext& ctx);

    [[nodiscard]] bool _bgm_test_track_is_playing() const;

    DebugScreen screen_;
    int cursor_;

    BgmId test_bgm_id_;
    SeId test_se_id_;
    bool last_drawn_bgm_playing_;

    bn::vector<bn::sprite_ptr, 128> sprites_;

    bool prev_sound_bgm_enabled_;
    bool prev_sound_se_enabled_;

    bn::fixed prev_sprite_intensity_;
    bool had_saved_sprite_intensity_;

    PhaseStep step_;
};

#endif
