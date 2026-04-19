# GBA Sokoban (Butano Edition) — CLAUDE.md

## プロジェクト概要

GBA向けの倉庫番（Sokoban）ゲームを、Cベースの旧実装（`../gba-sokoban/`）から
**Butano C++エンジン**へ移行した新実装。

- **エンジン**: [Butano](https://github.com/GValiente/butano) — GBA向けC++17フレームワーク
- **ターゲット**: Nintendo Game Boy Advance (ARM7TDMI)
- **ビルドシステム**: GNU Make + devkitARM (arm-none-eabi-gcc)
- **オーディオ**: Maxmod バックエンド
- **セーブ**: SRAM（32KB）

## ディレクトリ構成

```
gba-sokoban-bn/
├── Makefile              # Butanoのmakシステム（LIBBUTANO=../butano）
├── src/
│   ├── main.cpp          # エントリポイント・ゲームフロー管理
│   ├── state/            # ゲームステート（シーン）
│   │   ├── state.h           # State基底クラス（純粋仮想: init/update/shutdown）
│   │   ├── state_manager.h/cpp   # StateManagerスタック（bn::vector<State*,8>）
│   │   ├── title_state.*     # タイトル画面
│   │   ├── menu_state.*      # メインメニュー（STORY/ENDLESS/DEBUG/SETTINGS）
│   │   ├── puzzle_state.*    # ストーリーモードのパズル
│   │   ├── event_state.*     # ストーリーイベント（テキスト演出）
│   │   ├── endless_state.*   # エンドレスモード（生成パズル）
│   │   ├── settings_state.*  # 設定（SE on/off、テキスト速度）
│   │   └── debug_state.*     # デバッグメニュー（イベント/パズル直接起動）
│   ├── game/
│   │   ├── sokoban.h/cpp     # コアゲームロジック（GameState, game_init, game_move）
│   │   ├── puzzle_gen.h/cpp  # 手続き的パズル生成（difficulty 0-2, seed）
│   │   ├── event_script.h    # イベントスクリプトコマンド定義（EventCmd enum）
│   │   └── story_data.h      # ストーリースクリプトデータ（story_scripts[]配列）
│   ├── gfx/
│   │   ├── renderer.h/cpp    # BGマップへのゲーム盤面描画（render_draw_map）
│   │   └── hud.h/cpp         # HUD（手数表示）
│   ├── audio/
│   │   └── sound_manager.h/cpp   # SEラッパー（play_move/push/clear/reset）
│   └── save/
│       └── save_data.h/cpp   # SRAMセーブ（SaveData構造体、magic=0x534F4B42）
├── graphics/             # グラフィックアセット（grit処理）
│   └── bg.json           # BGタイル設定
└── audio/                # オーディオアセット（maxmod処理）
```

## アーキテクチャ

### ゲームフロー（main.cpp）

```
TITLE → MENU → {
    STORY:    STORY_EVENT → PUZZLE → STORY_EVENT → ... → MENU
    ENDLESS:  ENDLESS → MENU
    SETTINGS: SETTINGS → MENU
    DEBUG:    DEBUG → {直接イベント/パズル起動}
}
```

`GameFlow` enum と `StateManager` スタックで制御。
ステートが `pop()` されると `state_manager.empty()` が true になり、
main.cpp の switch 文でフロー遷移する。

### Stateインターフェース

```cpp
class State {
    virtual void init(StateManager& manager) = 0;    // 初期化・リソース確保
    virtual void update(StateManager& manager) = 0;  // 毎フレーム処理
    virtual void shutdown() = 0;                     // リソース解放
};
```

Butanoのリソース（`bn::regular_bg_ptr` など）は **shutdown() で必ず解放する**こと。
`bn::optional<>` を使ってReset可能にするのが定石。

### Butanoの制約（重要）

- **VRAM制限**: スプライト・BG枚数に厳しい上限あり
  - スプライト: 最大128枚
  - BGレイヤー: 最大4枚
- **スタックサイズ**: 非常に小さい（大きいローカル配列は避けること）
- **ヒープ**: Butano内部管理。`new`/`delete` は避け `bn::` コンテナを使う
- **テキスト描画**: `bn::sprite_text_generator` を使う。描画結果は `bn::vector<bn::sprite_ptr>` に保持
- **BGマップ**: `alignas(4) bn::array<bn::regular_bg_map_cell, 32*32>` をメンバに持つ
- **keypad入力**: `bn::keypad::a_pressed()` など `_pressed()` は1フレームのみtrue

### SaveData構造体

```cpp
struct SaveData {
    uint32_t magic;         // 0x534F4B42 ("SOKB")
    uint8_t  version;       // = 1
    bool     bgm_enabled;
    bool     se_enabled;
    uint8_t  text_speed;    // 0=slow, 1=normal, 2=fast
    uint8_t  story_chapter;
    uint16_t endless_high_score;
    uint8_t  flags[32];     // 256ビットのknowledgeフラグ
    uint8_t  _padding[2];
};
```

### EventScriptシステム

`event_state.h/cpp` がスクリプトインタープリタを実装。
コマンドは `EventCmd` enum（TEXT / WAIT_INPUT / SHOW_LEFT / GOTO_PUZZLE etc.）。
スクリプトは `story_data.h` にインラインで定義（`story_scripts[]` 配列）。

## ビルド方法

**WSL環境前提**（devkitARM + Butanoが必要）

```bash
# プロジェクトディレクトリで
cd /path/to/gba-sokoban-bn
make

# クリーンビルド
make clean && make

# 生成物
# gba-sokoban-bn.gba  ← 実行ファイル
# gba-sokoban-bn.elf
```

**Visual Studio経由**（Windowsのみ）:
`.sln`/`.vcxproj` ファイルが存在。VSからビルドするとWSL経由でmakeが走り、
成功時にmGBAエミュレータが起動する設定になっている。

## 旧実装（C版）との対応

旧実装は `../gba-sokoban/source/` にC言語で書かれている。
コアロジック（`game.c` の `game_move` / `game_init`）はほぼそのまま移植済み。

| 旧 (C) | 新 (Butano C++) |
|---|---|
| `state.h` の enum | `GameFlow` enum in main.cpp |
| `gfx.c` | `src/gfx/renderer.cpp` + `src/gfx/hud.cpp` |
| `sound.c` (maxmod直接) | `src/audio/sound_manager.cpp` |
| なし | `src/state/event_state.cpp` (新規) |
| なし | `src/game/puzzle_gen.cpp` (新規) |

## 現在の実装状況・残タスク

### ✅ 完了済み
- StateManager / State基底クラス
- TitleState（タイトル表示）
- MenuState（メニュー表示・選択）
- PuzzleState（ストーリーパズル実行）
- EventState（テキストイベント・スクリプトインタープリタ骨格）
- EndlessState（手続き生成パズル + スコア管理）
- DebugState（開発用メニュー）
- SettingsState（SE on/off等）
- SaveData / SRAM読み書き
- SoundManager（play_move/push/clear/reset）
- PuzzleGen（puzzle_generate: 難易度0-2, seed指定）
- story_data.h（Chapter 1 スクリプト 2本）

### 🔧 未完了・改善が必要な箇所

#### EventState の実装
- `draw_characters()` のキャラクター画像表示が未実装（placeholder状態）
- `SET_BG`, `FULL_STILL`, `CLEAR_STILL` コマンドが未実装
- テキストボックスのビジュアルがシンプルすぎる

#### ストーリーコンテンツ
- `story_data.h` に Chapter 2 以降が未記述
- キャラクターID / BG IDの定義が不完全

#### グラフィック
- キャラクタースプライトが未作成
- BGパターンが最小限

#### エンドレスモード
- `PuzzleGen` の生成ロジックが未検証（解けないパズルが生成される可能性）
- ハイスコア画面がシンプル

## 開発ガイドライン

1. **Butanoリソースは必ず `shutdown()` で解放する**（`bg_.reset()`, `sprites_.clear()` 等）
2. **大きなバッファはスタックに置かない** — クラスのメンバにするか static にする
3. **テキスト描画は毎フレーム行わない** — 変化があった時だけ再生成する
4. **`bn::optional<>` を活用** — 初期化前のBGポインタ等の管理に使う
5. **SRAMへの書き込みは控えめに** — 書き込み回数制限がある
