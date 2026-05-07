#ifndef ENDLESS_STATE_H
#define ENDLESS_STATE_H

#include "state/state.h"
#include "game/sokoban.h"
#include "graphics/renderer.h"
#include "ui/HUD/hud.h"
#include "audio/sound_manager.h"
#include "save/save_data.h"

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_array.h"
#include "bn_optional.h"
#include "bn_vector.h"
#include "ui/Core/Manager/ui_manager.h"

// エンドレスモードのフェーズ
enum class EndlessPhase {
    PLAYING,    // プレイ中
    RESULT,     // スコア表示中
    COUNT
};

class EndlessState : public State {
public:
    EndlessState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void change_phase(EndlessPhase next);

    void enter_playing();
    void update_playing(StateManager& sm, SharedContext& ctx);
    void exit_playing();

    void enter_result();
    void update_result(StateManager& sm, SharedContext& ctx);
    void exit_result();

    void generate_next();
    void draw_result(SharedContext& ctx);

    using EnterExitFunc = void (EndlessState::*)();
    using UpdateFunc = void (EndlessState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];

    Hud hud_;
    GameState gs_;

    int score_;
    int difficulty_;
    int seed_;

    bn::optional<bn::regular_bg_ptr> bg_;
    bn::optional<bn::regular_bg_map_ptr> bg_map_;
    alignas(4) bn::array<bn::regular_bg_map_cell, 32 * 32> map_cells_;

    bn::vector<bn::sprite_ptr, 32> result_sprites_;
    bn::optional<UIManager> ui_manager_;
    EndlessPhase phase_;
    PhaseStep    step_;
};

#endif // ENDLESS_STATE_H
