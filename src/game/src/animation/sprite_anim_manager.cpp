#include "animation/sprite_anim_manager.h"
#include "generated/sprite_anim_dispatch.gen.h"
#include "audio/sound_manager.h"
#include "bn_sprite_tiles_ptr.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
SpriteAnimManager& SpriteAnimManager::instance() {
    static SpriteAnimManager s;
    return s;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
AnimHandle SpriteAnimManager::play(SpriteAnimId id, bn::fixed x, bn::fixed y) {
    const int anim_idx = static_cast<int>(id);
    if (anim_idx < 0 || anim_idx >= static_cast<int>(SpriteAnimId::COUNT)) {
        return INVALID_ANIM_HANDLE;
    }
    return _play_internal(id, x, y, g_sprite_anims[anim_idx].default_loop_count);
}

AnimHandle SpriteAnimManager::play(SpriteAnimId id, bn::fixed x, bn::fixed y, int loop_count) {
    return _play_internal(id, x, y, loop_count);
}

void SpriteAnimManager::stop(AnimHandle handle) {
    if (auto* inst = _find(handle)) {
        inst->clear();
    }
}

void SpriteAnimManager::stop_all() {
    for (auto& inst : instances_) {
        inst.clear();
    }
}

bool SpriteAnimManager::is_playing(AnimHandle handle) const {
    const auto* inst = _find(handle);
    return inst != nullptr && inst->active;
}

void SpriteAnimManager::set_position(AnimHandle handle, bn::fixed x, bn::fixed y) {
    if (auto* inst = _find(handle)) {
        if (inst->sprite) {
            inst->sprite->set_position(x, y);
        }
    }
}

void SpriteAnimManager::set_bg_priority(AnimHandle handle, int priority) {
    if (auto* inst = _find(handle)) {
        if (inst->sprite) {
            inst->sprite->set_bg_priority(priority);
        }
    }
}

// ---------------------------------------------------------------------------
// update() — call once per frame from main loop / StateManager
// ---------------------------------------------------------------------------
void SpriteAnimManager::update() {
    for (auto& inst : instances_) {
        if (!inst.active || !inst.sprite) continue;

        const FdSpriteAnim& anim = g_sprite_anims[static_cast<int>(inst.anim_id)];
        if (inst.clip_index >= anim.clip_count) { inst.clear(); continue; }

        const FdSpriteClip& clip = anim.clips[inst.clip_index];

        inst.frame_timer++;
        if (inst.frame_timer < clip.frame_duration) continue;

        inst.frame_timer = 0;
        inst.frame_index++;

        if (inst.frame_index < clip.frame_count) {
            // 同じクリップ内の次コマへ
            const auto& item = get_sprite_item_for_clip(inst.anim_id, inst.clip_index);
            if (inst.frame_index < item.tiles_item().graphics_count()) {
                inst.sprite->set_tiles(item.tiles_item().create_tiles(inst.frame_index));
            }
        } else {
            // クリップ終端 → 次クリップへ
            inst.frame_index = 0;
            inst.clip_index++;

            if (inst.clip_index < anim.clip_count) {
                // 次のクリップに切り替え（スプライトを再生成）
                bn::fixed px = inst.sprite->x();
                bn::fixed py = inst.sprite->y();
                int  prio    = inst.sprite->bg_priority();
                _advance_to_clip(inst, px, py);
                inst.sprite->set_bg_priority(prio);
            } else {
                // シーケンス1周終了
                inst.clip_index = 0;

                if (inst.loop_remain < 0) {
                    // 無限ループ: 最初のクリップに戻る
                    bn::fixed px = inst.sprite->x();
                    bn::fixed py = inst.sprite->y();
                    int  prio    = inst.sprite->bg_priority();
                    _advance_to_clip(inst, px, py);
                    inst.sprite->set_bg_priority(prio);
                } else if (inst.loop_remain > 1) {
                    inst.loop_remain--;
                    bn::fixed px = inst.sprite->x();
                    bn::fixed py = inst.sprite->y();
                    int  prio    = inst.sprite->bg_priority();
                    _advance_to_clip(inst, px, py);
                    inst.sprite->set_bg_priority(prio);
                } else {
                    // 再生完了
                    inst.clear();
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
AnimHandle SpriteAnimManager::_play_internal(SpriteAnimId id, bn::fixed x, bn::fixed y, int loop_count) {
    const int anim_idx = static_cast<int>(id);
    if (anim_idx < 0 || anim_idx >= static_cast<int>(SpriteAnimId::COUNT)) {
        return INVALID_ANIM_HANDLE;
    }
    const FdSpriteAnim& anim = g_sprite_anims[anim_idx];
    if (anim.clip_count == 0) return INVALID_ANIM_HANDLE;

    // 空きスロットを探す
    AnimInstance* slot = nullptr;
    for (auto& inst : instances_) {
        if (!inst.active) { slot = &inst; break; }
    }
    if (!slot) return INVALID_ANIM_HANDLE; // 満杯

    slot->active      = true;
    slot->handle_id   = next_handle_++;
    slot->anim_id     = id;
    slot->clip_index  = 0;
    slot->frame_index = 0;
    slot->frame_timer = 0;
    slot->loop_remain = loop_count;

    // スプライト生成
    const auto& item = get_sprite_item_for_clip(id, 0);
    slot->sprite = item.create_sprite(x, y);

    // SE 再生
    if (anim.se_id != SeId::COUNT) {
        SoundManager::instance().play_se(anim.se_id);
    }

    return slot->handle_id;
}

SpriteAnimManager::AnimInstance* SpriteAnimManager::_find(AnimHandle handle) {
    if (handle == INVALID_ANIM_HANDLE) return nullptr;
    for (auto& inst : instances_) {
        if (inst.active && inst.handle_id == handle) return &inst;
    }
    return nullptr;
}

const SpriteAnimManager::AnimInstance* SpriteAnimManager::_find(AnimHandle handle) const {
    if (handle == INVALID_ANIM_HANDLE) return nullptr;
    for (const auto& inst : instances_) {
        if (inst.active && inst.handle_id == handle) return &inst;
    }
    return nullptr;
}

void SpriteAnimManager::_advance_to_clip(AnimInstance& inst, bn::fixed x, bn::fixed y) {
    inst.sprite.reset();
    const auto& item = get_sprite_item_for_clip(inst.anim_id, inst.clip_index);
    inst.sprite = item.create_sprite(x, y);
    inst.frame_index = 0;
    inst.frame_timer = 0;
}
