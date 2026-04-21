# GBA Sokoban (Butano C++ Engine) - AI Coding Rules

## 1. ターゲット環境
- Nintendo Game Boy Advance (ARM7TDMI)
- C++17 / Butano Engine (https://github.com/GValiente/butano)

## 2. 絶対厳守のハードウェア＆エンジン制約 (CRITICAL)
- **リソース解放:** Butanoリソース(`bn::regular_bg_ptr`等)は、Stateの `shutdown()` で必ず解放すること。`bn::optional<>` を使いReset可能にするのが定石。
- **メモリ管理:** VRAM制限・スタックサイズが極めて小さい。ローカル配列は避け、`new/delete` の代わりに `bn::` コンテナを使用すること。
- **テキスト描画:** `bn::sprite_text_generator` を使用し、毎フレームの再生成は避ける（変化があった時のみ）。
- **入力:** `bn::keypad::a_pressed()` など `_pressed()` は1フレームのみtrueになる仕様に注意。

## 3. アーキテクチャの前提
- **State管理:** `StateManager` スタック (bn::vector<State*,8>) で制御。State基底は `init()`, `update()`, `shutdown()` を持つ。
- **データセーブ:** SRAM（32KB）を使用。`SaveData` 構造体への書き込みは寿命を考慮し最小限に留めること。