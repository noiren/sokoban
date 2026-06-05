#ifndef USER_DATA_PUZZLE_H
#define USER_DATA_PUZZLE_H

#include <cstdint>

// パズルの最大ステージ数（将来の拡張余地込み）
constexpr int MAX_PUZZLE_RECORDS = 16;

// 1ステージ分のプレイ記録
struct PuzzleRecord {
    bool     attempted;     // 挑戦したことがあるか
    bool     story_cleared; // ストーリーモードでクリアしたか
    uint16_t min_moves;     // 最小手数（0 = 未記録）
    uint16_t best_frames;   // 最短クリア時間（フレーム数）（0 = 未記録）
    uint8_t  _padding[2];   // 4バイトアライメント用
};

static_assert(sizeof(PuzzleRecord) % 4 == 0, "PuzzleRecord must be 4-byte aligned");

// パズルユーザーデータ全体
struct UserDataPuzzle {
    uint32_t     magic;                          // PUZZLE_DATA_MAGIC で有効判定
    uint8_t      version;                        // バージョン
    uint8_t      _padding[3];                    // アライメント
    PuzzleRecord records[MAX_PUZZLE_RECORDS];    // ステージごとの記録
};

static_assert(sizeof(UserDataPuzzle) % 4 == 0, "UserDataPuzzle must be 4-byte aligned");

constexpr uint32_t PUZZLE_DATA_MAGIC   = 0x50555A4C; // "PUZL"
constexpr uint8_t  PUZZLE_DATA_VERSION = 2;

// 初期化
void user_data_puzzle_init(UserDataPuzzle& data);

// 有効判定
bool user_data_puzzle_is_valid(const UserDataPuzzle& data);

// SRAM 読み書き（SaveData の後ろのオフセットに保存）
void user_data_puzzle_load(UserDataPuzzle& data);
void user_data_puzzle_save(const UserDataPuzzle& data);

// ステージのクリア記録を更新（ベストなら true を返す）
bool user_data_puzzle_update(UserDataPuzzle& data, int level, int moves, int frames);

// 挑戦済みフラグのみ立てる（クリアしていなくても）
void user_data_puzzle_set_attempted(UserDataPuzzle& data, int level);

// ストーリークリア済みフラグを立てる
void user_data_puzzle_set_story_cleared(UserDataPuzzle& data, int level);

#endif // USER_DATA_PUZZLE_H
