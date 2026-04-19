#include "sound_manager.h"
#include "bn_sound_items.h"

void SoundManager::play_move() {
    if (se_enabled_) bn::sound_items::supi.play();
}

void SoundManager::play_push() {
    if (se_enabled_) bn::sound_items::jowayo.play();
}

void SoundManager::play_clear() {
    if (se_enabled_) bn::sound_items::jowayo3.play();
}

void SoundManager::play_reset() {
    if (se_enabled_) bn::sound_items::uwa.play();
}

void SoundManager::set_se_enabled(bool enabled) {
    se_enabled_ = enabled;
}

bool SoundManager::se_enabled() const {
    return se_enabled_;
}
