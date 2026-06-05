#include "user_data_puzzle.h"
#include "save_data.h"
#include "bn_sram.h"

// UserDataPuzzle は SaveData の直後のオフセットに保存する
static constexpr int PUZZLE_DATA_SRAM_OFFSET = static_cast<int>(sizeof(SaveData));

void user_data_puzzle_init(UserDataPuzzle& data) {
    data.magic      = PUZZLE_DATA_MAGIC;
    data.version    = PUZZLE_DATA_VERSION;
    data._padding[0] = 0;
    data._padding[1] = 0;
    data._padding[2] = 0;
    for (int i = 0; i < MAX_PUZZLE_RECORDS; ++i) {
        data.records[i].attempted     = false;
        data.records[i].story_cleared = false;
        data.records[i].min_moves     = 0;
        data.records[i].best_frames   = 0;
        data.records[i]._padding[0]   = 0;
        data.records[i]._padding[1]   = 0;
    }
}

bool user_data_puzzle_is_valid(const UserDataPuzzle& data) {
    return data.magic == PUZZLE_DATA_MAGIC && data.version == PUZZLE_DATA_VERSION;
}

void user_data_puzzle_load(UserDataPuzzle& data) {
    bn::sram::read_offset(data, PUZZLE_DATA_SRAM_OFFSET);
    if (!user_data_puzzle_is_valid(data)) {
        user_data_puzzle_init(data);
        user_data_puzzle_save(data);
    }
}

void user_data_puzzle_save(const UserDataPuzzle& data) {
    bn::sram::write_offset(data, PUZZLE_DATA_SRAM_OFFSET);
}

bool user_data_puzzle_update(UserDataPuzzle& data, int level, int moves, int frames) {
    if (level < 0 || level >= MAX_PUZZLE_RECORDS) {
        return false;
    }

    PuzzleRecord& rec = data.records[level];
    rec.attempted = true;

    bool updated = false;

    // 最小手数の更新（未記録または改善された場合）
    if (rec.min_moves == 0 || moves < static_cast<int>(rec.min_moves)) {
        rec.min_moves = static_cast<uint16_t>(moves);
        updated = true;
    }

    // 最短フレームの更新（未記録または改善された場合）
    if (rec.best_frames == 0 || frames < static_cast<int>(rec.best_frames)) {
        rec.best_frames = static_cast<uint16_t>(frames < 65535 ? frames : 65535);
        updated = true;
    }

    return updated;
}

void user_data_puzzle_set_attempted(UserDataPuzzle& data, int level) {
    if (level < 0 || level >= MAX_PUZZLE_RECORDS) {
        return;
    }
    data.records[level].attempted = true;
}

void user_data_puzzle_set_story_cleared(UserDataPuzzle& data, int level) {
    if (level < 0 || level >= MAX_PUZZLE_RECORDS) {
        return;
    }
    data.records[level].attempted     = true;
    data.records[level].story_cleared = true;
}
