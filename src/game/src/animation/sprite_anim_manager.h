#ifndef SPRITE_ANIM_MANAGER_H
#define SPRITE_ANIM_MANAGER_H

#include "generated/sprite_anim_data.h"
#include "bn_fixed.h"
#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "bn_array.h"

using AnimHandle = int;
static constexpr AnimHandle INVALID_ANIM_HANDLE = -1;

class SpriteAnimManager {
public:
    static SpriteAnimManager& instance();

    // デフォルトloop_count（JSON定義通り）で再生
    AnimHandle play(SpriteAnimId id, bn::fixed x, bn::fixed y);

    // loop_countを上書きして再生 (-1=無限, 0=1回のみ, N=N回ループ)
    AnimHandle play(SpriteAnimId id, bn::fixed x, bn::fixed y, int loop_count);

    // 明示的に停止
    void stop(AnimHandle handle);

    // 全停止
    void stop_all();

    // 毎フレーム呼ぶ（StateManager::update内など）
    void update();

    // 再生中かどうか
    bool is_playing(AnimHandle handle) const;

    // 位置の更新
    void set_position(AnimHandle handle, bn::fixed x, bn::fixed y);

    // BGプライオリティの設定
    void set_bg_priority(AnimHandle handle, int priority);

private:
    SpriteAnimManager() = default;

    struct AnimInstance {
        bool active = false;
        int  handle_id = INVALID_ANIM_HANDLE;
        SpriteAnimId anim_id = SpriteAnimId::COUNT;

        int clip_index  = 0;  // 現在のクリップ番号
        int frame_index = 0;  // クリップ内コマ番号
        int frame_timer = 0;  // コマ内タイマー
        int loop_remain = 0;  // 残りループ回数 (-1=無限)

        bn::optional<bn::sprite_ptr> sprite;

        void clear() {
            active = false;
            handle_id = INVALID_ANIM_HANDLE;
            sprite.reset();
        }
    };

    static constexpr int MAX_INSTANCES = 16;
    bn::array<AnimInstance, MAX_INSTANCES> instances_;
    int next_handle_ = 0;

    AnimHandle _play_internal(SpriteAnimId id, bn::fixed x, bn::fixed y, int loop_count);
    AnimInstance* _find(AnimHandle handle);
    const AnimInstance* _find(AnimHandle handle) const;
    void _advance_to_clip(AnimInstance& inst, bn::fixed x, bn::fixed y);
};

#endif
