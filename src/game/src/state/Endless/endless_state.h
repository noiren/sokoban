#ifndef ENDLESS_STATE_H
#define ENDLESS_STATE_H

#include "state/state.h"
#include "game/puzzle_gen.h"
#include "game/puzzle_engine.h"
#include "game/sokoban.h"

#include "graphics/renderer.h"

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_camera_ptr.h"
#include "bn_array.h"
#include "bn_optional.h"
#include "ui/Core/Manager/ui_manager.h"

// エンドレス画面の大フェーズ
enum class EndlessPhase {
    PLAYING,
    RESULT,
    COUNT
};

// プレイ中のパズル進行（PuzzleState と同様の内側フェーズ）
enum class EndlessPuzzlePhase {
    PP_PLAYING,
    PP_ANIMATING,
    PP_FAILED,
    PP_CLEARED,
    COUNT
};

class EndlessState : public State {
public:
    EndlessState();
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}
    void resume(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}

private:
    void change_phase(EndlessPhase next);
    void change_puzzle_phase(EndlessPuzzlePhase next);

    void enter_playing();
    void update_playing(StateManager& sm, SharedContext& ctx);
    void exit_playing();

    void enter_result();
    void update_result(StateManager& sm, SharedContext& ctx);
    void exit_result();

    void enter_pp_playing();
    void update_pp_playing(StateManager& sm, SharedContext& ctx);
    void exit_pp_playing();

    void enter_pp_animating();
    void update_pp_animating(StateManager& sm, SharedContext& ctx);
    void exit_pp_animating();

    void enter_pp_failed();
    void update_pp_failed(StateManager& sm, SharedContext& ctx);
    void exit_pp_failed();

    void enter_pp_cleared();
    void update_pp_cleared(StateManager& sm, SharedContext& ctx);
    void exit_pp_cleared();

    void generate_next();
    void reload_last_generated();
    void ensure_map_resources(SharedContext& ctx);
    void refresh_rival_portrait_ui();
    void redraw_map();
    void update_hud();
    void get_visual_player_pos(bn::fixed& px, bn::fixed& py);
    void update_camera();
    void update_player_sprite();

    using EnterExitFunc = void (EndlessState::*)();
    using UpdateFunc    = void (EndlessState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    struct PuzzlePhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];
    static const PuzzlePhaseHandlers puzzle_phase_table_[];

    bn::optional<UIManager> ui_manager_;

    PuzzleEngine engine_;
    EndlessPhase       phase_;
    EndlessPuzzlePhase puzzle_phase_;
    PhaseStep          step_;

    PuzzleEngine::Result last_result_;

    int score_;
    int difficulty_;
    int seed_;

    int last_drawn_moves_;
    int current_event_index_;

    int move_anim_frames_     = 0;
    int move_anim_max_frames_ = 8;
    int move_src_x_           = 0;
    int move_src_y_           = 0;
    int move_dst_x_           = 0;
    int move_dst_y_           = 0;
    int player_dir_           = 0;

    GameState last_generated_{};

    bn::optional<bn::regular_bg_ptr>     bg_;
    bn::optional<bn::regular_bg_map_ptr> bg_map_;
    bn::optional<bn::camera_ptr>        camera_;
    alignas(4) bn::array<bn::regular_bg_map_cell, 64 * 64> map_cells_;

    bn::optional<bn::sprite_ptr>                      player_sprite_;
    bn::optional<bn::sprite_animate_action<4>>      player_anim_;
};

#endif // ENDLESS_STATE_H
