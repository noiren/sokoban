#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

class SoundManager {
public:
    void play_move();
    void play_push();
    void play_clear();
    void play_reset();

    void set_se_enabled(bool enabled);
    bool se_enabled() const;

private:
    bool se_enabled_ = true;
};

#endif // SOUND_MANAGER_H
