#ifndef SAVE_DATA_H
#define SAVE_DATA_H

#include <cstdint>

// セーブスロット数
constexpr int NUM_SAVE_SLOTS = 3;

// 1スロット分のセーブデータ
struct SaveSlot {
    uint32_t magic;              // SAVE_MAGIC で有効判定
    uint8_t  version;

    // 設定
    bool     bgm_enabled;
    bool     se_enabled;
    uint8_t  text_speed;         // 0=slow, 1=normal, 2=fast

    // 進捗
    uint8_t  story_chapter_idx;   // 現在のチャプターインデックス
    uint8_t  story_step_idx;      // チャプター内のステップインデックス
    uint16_t endless_high_score;
    bool     autosave_enabled;    // オートセーブON/OFF

    // フラグ (256ビット = 256フラグ)
    uint8_t  flags[32];

    // パディング (4バイトアライメント維持)
    uint8_t  _padding[2];
};

static_assert(sizeof(SaveSlot) % 4 == 0, "SaveSlot must be 4-byte aligned");

// SRAM 全体構造（3スロット）
struct SaveData {
    SaveSlot slots[NUM_SAVE_SLOTS];
};

// 後方互換のために現在アクティブなスロットを参照する別名
// （既存コードが SaveData& を受け取るケースへの移行用）
using SaveSlotRef = SaveSlot&;

constexpr uint32_t SAVE_MAGIC   = 0x534F4B42; // "SOKB"
constexpr uint8_t  SAVE_VERSION = 3;

// 1スロットを初期値で初期化
void save_slot_init(SaveSlot& slot);

// スロットが有効なデータを持つか
bool save_slot_is_valid(const SaveSlot& slot);

// SRAM 読み書き（全スロット）
bool save_data_load(SaveData& data);
void save_data_save(const SaveData& data);

// 特定スロットだけ書き込む
void save_slot_save(const SaveData& data, int slot_index);

// フラグ操作（スロット指定）
bool save_slot_get_flag(const SaveSlot& slot, int flag_id);
void save_slot_set_flag(SaveSlot& slot, int flag_id, bool value);

// ---- 後方互換ラッパー（既存コードが壊れないように） ----
// 将来的には削除し、スロットを直接渡す形に移行する
inline bool save_data_get_flag(const SaveSlot& slot, int flag_id) {
    return save_slot_get_flag(slot, flag_id);
}
inline void save_data_set_flag(SaveSlot& slot, int flag_id, bool value) {
    save_slot_set_flag(slot, flag_id, value);
}

#endif // SAVE_DATA_H
