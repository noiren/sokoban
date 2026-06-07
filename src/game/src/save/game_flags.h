#ifndef GAME_FLAGS_H
#define GAME_FLAGS_H

// ============================================================
// SaveSlot.flags[32] で管理するフラグIDの定義
// 256ビット (flag_id 0〜255) が使用可能
// ============================================================

// --- モード解禁フラグ (0〜31) ---
constexpr int FLAG_ENDLESS_UNLOCKED  = 0;   // エンドレスモード解禁
constexpr int FLAG_PRACTICE_UNLOCKED = 1;   // プラクティス（ストーリークリア済みステージの再挑戦）
constexpr int FLAG_GALLERY_UNLOCKED  = 2;   // ギャラリー解禁（将来用）

// --- ギャラリーアイテム解禁フラグ (32〜) ---
// g_gallery[i] に対応するフラグ = FLAG_GALLERY_ITEM_BASE + i
// ギャラリーアイテムが増えてもここは変えなくてよい
constexpr int FLAG_GALLERY_ITEM_BASE = 32;

// FLAG_GALLERY_ITEM_BASE + index でギャラリーアイテムを参照
// unlock_flag = -1 → 常時解禁
// unlock_flag >= 0 → save_slot_get_flag(slot, unlock_flag) == true で解禁

#endif // GAME_FLAGS_H
