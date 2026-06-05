#ifndef STILL_EVENT_STATE_H
#define STILL_EVENT_STATE_H

#include "state/state.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"
#include "ui/Core/Effects/fade_effect.h"
#include "generated/generated_fix_data.h"

enum class StillEventPhase {
    PAGE_ENTER,
    MSG_FADE_IN,
    MSG_WAIT,
    MSG_FADE_OUT,
    PAGE_EXIT,
    COUNT
};

class StillEventState : public State {
public:
    static constexpr int FADE_DURATION = 16;
    static constexpr int MSG_FADE_DURATION = 12;

    StillEventState();

    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}
    void resume(StateManager& /*sm*/, SharedContext& /*ctx*/) override {}

private:
    void change_phase(StillEventPhase next);

    void enter_page_enter();
    void update_page_enter(StateManager& sm, SharedContext& ctx);
    void exit_page_enter();

    void enter_msg_fade_in();
    void update_msg_fade_in(StateManager& sm, SharedContext& ctx);
    void exit_msg_fade_in();

    void enter_msg_wait();
    void update_msg_wait(StateManager& sm, SharedContext& ctx);
    void exit_msg_wait();

    void enter_msg_fade_out();
    void update_msg_fade_out(StateManager& sm, SharedContext& ctx);
    void exit_msg_fade_out();

    void enter_page_exit();
    void update_page_exit(StateManager& sm, SharedContext& ctx);
    void exit_page_exit();

    void apply_current_page(SharedContext& ctx);
    void apply_current_msg(SharedContext& ctx);
    void refresh_display_text(SharedContext& ctx);
    void set_text_alpha(int alpha); // 0-16 for fade

    using EnterExitFunc = void (StillEventState::*)();
    using UpdateFunc    = void (StillEventState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];

    const FdStillEventEntry* event_entry_;
    int page_idx_;
    int msg_idx_;
    StillEventPhase phase_;
    
    int fade_timer_;
    int fade_timer_max_;

    bn::vector<bn::sprite_ptr, 64> text_sprites_;
    FadeEffect fade_;
    bn::optional<bn::regular_bg_ptr> still_bg_;
};

#endif // STILL_EVENT_STATE_H
