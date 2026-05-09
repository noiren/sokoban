#include "sound_manager.h"

#include "generated/audio_dispatch_bgm.gen.h"
#include "generated/audio_dispatch_se.gen.h"

#include "bn_fixed.h"
#include "bn_music.h"

SoundManager& SoundManager::instance()
{
    static SoundManager inst;
    return inst;
}

bn::fixed SoundManager::_target_bgm_volume() const
{
    return bgm_enabled_ ? bn::fixed(1) : bn::fixed(0);
}

void SoundManager::_clear_fade()
{
    fade_total_frames_ = 0;
    fade_left_frames_ = 0;
    fade_stop_after_ = false;
}

void SoundManager::_stop_bgm_internal()
{
    bn::music::stop();
    _clear_fade();
}

void SoundManager::_tick_fade()
{
    if (fade_total_frames_ == 0) {
        return;
    }

    if (! bn::music::playing()) {
        _clear_fade();
        return;
    }

    if (fade_left_frames_ > 0) {
        --fade_left_frames_;
    }

    const int progressed = fade_total_frames_ - fade_left_frames_;
    bn::fixed t = bn::fixed(progressed) / bn::fixed(fade_total_frames_);
    bn::fixed v = fade_start_vol_ + (fade_end_vol_ - fade_start_vol_) * t;
    bn::music::set_volume(v);

    if (fade_left_frames_ <= 0) {
        if (fade_stop_after_) {
            bn::music::stop();
        }
        _clear_fade();
    }
}

void SoundManager::update()
{
    _tick_fade();
}

void SoundManager::play_bgm(BgmId id, bool loop, int fade_in_frames, bool force_restart)
{
    if (BN_GENERATED_BGM_COUNT == 0) {
        return;
    }

    if (! audio_dispatch::bgm_id_valid(id)) {
        return;
    }

    if (! bgm_enabled_) {
        return;
    }

    const bn::music_item item = audio_dispatch::bgm_item(id);

    if (! force_restart && bn::music::playing()) {
        bn::optional<bn::music_item> cur = bn::music::playing_item();
        if (cur.has_value() && cur.value() == item) {
            return;
        }
    }

    _clear_fade();
    bn::music::stop();

    const bn::fixed start_vol = (fade_in_frames > 0) ? bn::fixed(0) : _target_bgm_volume();
    bn::music::play(item, start_vol, loop);

    if (fade_in_frames > 0) {
        fade_total_frames_ = fade_in_frames;
        fade_left_frames_ = fade_in_frames;
        fade_start_vol_ = bn::fixed(0);
        fade_end_vol_ = _target_bgm_volume();
        fade_stop_after_ = false;
    }
}

void SoundManager::stop_bgm(int fade_out_frames)
{
    if (BN_GENERATED_BGM_COUNT == 0) {
        return;
    }

    if (! bn::music::playing()) {
        _clear_fade();
        return;
    }

    if (fade_out_frames <= 0) {
        _stop_bgm_internal();
        return;
    }

    fade_total_frames_ = fade_out_frames;
    fade_left_frames_ = fade_out_frames;
    fade_start_vol_ = bn::music::volume();
    fade_end_vol_ = bn::fixed(0);
    fade_stop_after_ = true;
}

void SoundManager::play_se(SeId id)
{
    if (BN_GENERATED_SE_COUNT == 0) {
        return;
    }

    if (! se_enabled_) {
        return;
    }

    if (! audio_dispatch::se_id_valid(id)) {
        return;
    }

    audio_dispatch::se_item(id).play();
}

void SoundManager::set_bgm_enabled(bool enabled)
{
    bgm_enabled_ = enabled;

    if (! bgm_enabled_ && bn::music::playing()) {
        _stop_bgm_internal();
    }
}

bool SoundManager::bgm_enabled() const
{
    return bgm_enabled_;
}

void SoundManager::set_se_enabled(bool enabled)
{
    se_enabled_ = enabled;
}

bool SoundManager::se_enabled() const
{
    return se_enabled_;
}

void SoundManager::play_move()
{
    play_se(SeId::Default_Move);
}

void SoundManager::play_push()
{
    play_se(SeId::Default_Push);
}

void SoundManager::play_clear()
{
    play_se(SeId::Default_Clear);
}

void SoundManager::play_reset()
{
    play_se(SeId::Default_Reset);
}
