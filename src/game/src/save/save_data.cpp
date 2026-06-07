#include "save_data.h"
#include "save/game_flags.h"
#include "bn_sram.h"

// ==============================
// スロット単体操作
// ==============================

void save_slot_init(SaveSlot& slot) {
    slot.magic               = SAVE_MAGIC;
    slot.version             = SAVE_VERSION;
    slot.bgm_enabled         = true;
    slot.se_enabled          = true;
    slot.text_speed          = 1;  // normal
    slot.story_chapter_idx   = 0;
    slot.story_step_idx      = 0;
    slot.endless_high_score  = 0;
    slot.autosave_enabled    = false;
    for (int i = 0; i < 32; i++) {
        slot.flags[i] = 0;
    }
    // 既存仕様どおりプラクティスはデフォルト解禁（ロックしたい場合はフラグを落とす）
    save_slot_set_flag(slot, FLAG_PRACTICE_UNLOCKED, true);
    slot._padding[0] = 0;
}

bool save_slot_is_valid(const SaveSlot& slot) {
    return slot.magic == SAVE_MAGIC && slot.version == SAVE_VERSION;
}

// ==============================
// SRAM 読み書き
// ==============================

bool save_data_load(SaveData& data) {
    bn::sram::read(data);
    bool any_valid = false;
    bool migrated_flags = false;
    for (int i = 0; i < NUM_SAVE_SLOTS; i++) {
        if (!save_slot_is_valid(data.slots[i])) {
            save_slot_init(data.slots[i]);
        } else {
            any_valid = true;
            // 旧セーブ（フラグ未使用時代）: メニューと挙動を一致させる
            if (!save_slot_get_flag(data.slots[i], FLAG_PRACTICE_UNLOCKED)) {
                save_slot_set_flag(data.slots[i], FLAG_PRACTICE_UNLOCKED, true);
                migrated_flags = true;
            }
        }
    }
    if (!any_valid) {
        // 全スロット不正 → 全初期化して書き込む
        save_data_save(data);
    } else if (migrated_flags) {
        save_data_save(data);
    }
    return any_valid;
}

void save_data_save(const SaveData& data) {
    bn::sram::write(data);
}

void save_slot_save(const SaveData& data, int slot_index) {
    // スロット1件だけSRAMに書く（オフセット計算）
    // Butano の partial write: sram::write_offset 相当
    // 現状は全体書き込みにフォールバック
    // TODO: partial writeに最適化
    bn::sram::write(data);
    (void)slot_index;
}

// ==============================
// フラグ操作
// ==============================

bool save_slot_get_flag(const SaveSlot& slot, int flag_id) {
    if (flag_id < 0 || flag_id >= 256) return false;
    int byte_idx = flag_id / 8;
    int bit_idx  = flag_id % 8;
    return (slot.flags[byte_idx] >> bit_idx) & 1;
}

void save_slot_set_flag(SaveSlot& slot, int flag_id, bool value) {
    if (flag_id < 0 || flag_id >= 256) return;
    int byte_idx = flag_id / 8;
    int bit_idx  = flag_id % 8;
    if (value) {
        slot.flags[byte_idx] |= (1 << bit_idx);
    } else {
        slot.flags[byte_idx] &= ~(1 << bit_idx);
    }
}
