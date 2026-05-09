#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "generated/audio_ids.h"

#include "bn_fixed.h"

class SoundManager {
public:
    static SoundManager& instance();

    void update();

    void play_bgm(BgmId id, bool loop, int fade_in_frames = 0, bool force_restart = false);
    void stop_bgm(int fade_out_frames = 0);

    void play_se(SeId id);

    void set_bgm_enabled(bool enabled);
    bool bgm_enabled() const;

    void set_se_enabled(bool enabled);
    bool se_enabled() const;

    void play_move();
    void play_push();
    void play_clear();
    void play_reset();

private:
    SoundManager() = default;

    [[nodiscard]] bn::fixed _target_bgm_volume() const;

    void _clear_fade();
    void _tick_fade();
    void _stop_bgm_internal();

    bool bgm_enabled_ = true;
    bool se_enabled_ = true;

    int fade_total_frames_ = 0;
    int fade_left_frames_ = 0;
    bn::fixed fade_start_vol_{0};
    bn::fixed fade_end_vol_{0};
    bool fade_stop_after_ = false;
};

#endif
