# GBA Sokoban (gba-sokoban-bn) プロジェクト構成まとめ

> 最終更新: 2026-04-22
> エンジン: [Butano](https://github.com/GValiente/butano) (C++17, GBA向けゲームライブラリ)
> ビルド: GNU Makefile + ARM GCC クロスコンパイラ

---

## ディレクトリ構成

```
gba-sokoban-bn/
├── src/                        # C++ ソースコード
│   ├── main.cpp                # エントリポイント・ゲームフロー制御
│   ├── state/                  # ステートマシン (各画面)
│   ├── game/                   # ゲームロジック
│   ├── gfx/                    # 描画・UI管理
│   ├── audio/                  # サウンド管理
│   └── save/                   # セーブデータ (SRAM)
├── include/                    # 共有ヘッダ
│   ├── ui_types.h              # UIデータ型定義
│   └── ui_data_*.h             # ★自動生成: 各画面のレイアウトデータ
├── graphics/                   # 画像アセット (.bmp + .json)
│   ├── *.bmp / *.json          # 背景・フォントなど
│   ├── stills/chara/           # キャラクタースチル画像
│   └── ui/menu/                # メニュー用スプライト
├── audio/                      # BGM・SE (maxmod)
├── dmg_audio/                  # DMGオーディオ (GameBoy風チャンネル)
├── tools/                      # 開発支援ツール群
│   ├── ui_compiler.py          # ★ビルド時自動実行: JSON→C++ヘッダ変換
│   ├── generate_assets.py      # アセット生成スクリプト
│   ├── generate_assets.exe     # ↑のスタンドアロン版
│   ├── assets_list.json        # アセット定義リスト
│   ├── image_manifest.json     # image_set→ファイルパス マッピング
│   ├── ui_editor/index.html    # Webベース UIレイアウトエディタ
│   └── asset_editor/index.html # Webベース アセット管理エディタ
├── ui/screens/                 # UIレイアウト定義 JSON (エディタ出力)
├── Makefile                    # ビルド設定
├── HANDOVER.md                 # セッション引き継ぎメモ
└── PROJECT_OVERVIEW.md         # 本ファイル
```

---

## アーキテクチャ概要

### 1. ゲームフロー (main.cpp)

スタックベースの `StateManager` で画面遷移を管理。  
`GameFlow` 列挙子によって遷移先を決定する。

```
TITLE → SAVE_SELECT → MENU ─┬─ STORY → STORY_EVENT ↔ PUZZLE
                              ├─ PRACTICE → (練習レベル選択)
                              ├─ ENDLESS → (無限生成パズル)
                              ├─ GALLERY → (スチル閲覧)
                              ├─ SETTINGS → (設定)
                              └─ DEBUG → (開発者メニュー)
```

GBAはヒープを使わないため、**全 State オブジェクトを main() のスタック上に確保**している。

### 2. ステート一覧 (src/state/)

| ファイル | 役割 |
|---|---|
| `state.h` | State 基底インターフェース |
| `state_manager.h/cpp` | スタックベースの State 管理 |
| `title_state` | タイトル画面 |
| `save_select_state` | セーブスロット選択 |
| `menu_state` | メインメニュー (STORY / PRACTICE / ENDLESS / GALLERY / SETTINGS / DEBUG) |
| `puzzle_state` | 倉庫番パズル本編 |
| `event_state` | ビジュアルノベル風イベント再生 |
| `endless_state` | 無限パズル生成モード |
| `practice_menu_state` | 練習モード (レベル選択) |
| `gallery_state` | スチル画像ギャラリー |
| `settings_state` | 設定画面 (BGM/SE/テキスト速度) |
| `debug_state` | 開発者用デバッグメニュー |

### 3. UIシステム (データ駆動型)

#### データの流れ

```
ui/screens/*.json     ← UIエディタ (tools/ui_editor/) で編集
       ↓ [ビルド時] tools/ui_compiler.py (Makefileの EXTTOOL で自動実行)
include/ui_data_*.h   ← C++定数データとして出力 (AUTO-GENERATED, 編集禁止)
       ↓ [実行時]
UIManager::load_screen()  ← ScreenData を受け取りスプライト/BG/テキストを展開
```

#### 主要な型 (include/ui_types.h)

```cpp
namespace ui_types {
    struct SpriteEntry { id, image_set, image_no, x, y, visible };
    struct TextEntry   { id, text, x, y, center_align, blink, blink_interval, visible };
    struct ScreenData  { bg_image_id, bg_scroll_x/y, sprites[], texts[] };
}
```

#### image_set / image_no マッピング (tools/image_manifest.json)

| image_set | no | ファイルパス |
|---|---|---|
| `menu_items` | 0 | `ui/menu/spr_menu_paper` |
| `menu_items` | 1 | `ui/menu/spr_menu_icon_story` |
| `menu_items` | 2 | `ui/menu/spr_menu_icon_practice` |
| `menu_items` | 3 | `ui/menu/spr_menu_icon_endless` |
| `chara_stills` | 0 | `stills/chara/spr_chara_default` |
| `chara_stills` | 1 | `stills/chara/spr_chara_smile` |
| `chara_stills` | 2 | `stills/chara/spr_chara_sad` |
| `dummy` | 0 | `spr_dummy` |

#### UIManager (src/gfx/ui_manager.h/cpp)

- `load_screen(ScreenData&)` : 画面データをロードし BG/スプライト/テキストを生成
- `update()` : 毎フレーム呼び出し。テキスト点滅などを処理
- `clear_all()` : 全リソース解放
- 内部でスプライトは最大16個、テキストは最大16個まで保持

### 4. イベントスクリプト (src/game/)

ビジュアルノベル風のイベントを `EventCommand` 配列で定義。

```cpp
enum class EventCmd {
    TEXT, WAIT_INPUT, CLEAR_TEXT,         // テキスト
    SHOW_LEFT, SHOW_RIGHT,                // キャラ表示
    HIDE_LEFT, HIDE_RIGHT,               // キャラ非表示
    SET_BG, CLEAR_BG,                    // 背景
    FULL_STILL, CLEAR_STILL,             // フルスクリーンスチル
    SET_FLAG, CHECK_FLAG,                // フラグ制御
    GOTO_PUZZLE,                          // パズルへ移行
    END, PLAY_SE                         // 終了・SE
}
```

ストーリーデータは `src/game/story_data.h` にC++配列として直書き。  
現在: チャプター1イントロ + チャプター1クリア後の2スクリプトのみ実装。

### 5. セーブデータ (src/save/)

SRAM (GBA内蔵の不揮発メモリ) に保存。スロット数: **3**

```
SaveData
└── SaveSlot[3]
    ├── magic (0x534F4B42 = "SOKB")  ← 有効判定
    ├── version (現在: 2)
    ├── bgm_enabled, se_enabled, text_speed
    ├── story_chapter, story_level
    ├── endless_high_score
    └── flags[32]  (256ビットのフラグ領域)
```

### 6. グラフィックアセット (graphics/)

Butano が `.bmp` + `.json` ペアを自動処理してC++ヘッダを生成する。

| ファイル | 用途 |
|---|---|
| `bg.bmp` | 汎用背景 |
| `bg_logo.bmp` | ロゴ画面背景 |
| `bg_title.bmp` | タイトル画面背景 |
| `common_fixed_8x8_font.bmp` | 英数字フォント (8x8固定幅) |
| `common_fixed_8x16_font.bmp` | 英数字フォント (8x16固定幅) |
| `japanese_font.bmp` | 日本語フォント (可変幅対応) |
| `spr_dummy.bmp` | ダミースプライト (32x32) |
| `stills/chara/` | キャラクタースチル (未実装/ダミー) |
| `ui/menu/` | メニュー用スプライト (paper, icons等) |

---

## ビルド設定 (Makefile)

| 項目 | 値 |
|---|---|
| `TARGET` | `gba-sokoban-bn` |
| `LIBBUTANO` | `../butano` |
| `SOURCES` | `src src/state src/game src/gfx src/audio src/save` |
| `GRAPHICS` | `graphics graphics/stills/chara graphics/ui/menu` |
| `AUDIO` | `audio` (maxmod バックエンド) |
| `DMGAUDIO` | `dmg_audio` (default バックエンド) |
| `EXTTOOL` | `python tools/ui_compiler.py` ← ビルド前に自動実行 |
| `ROMTITLE` | `SOKOBAN` |
| `ROMCODE` | `SOKB` |

> **重要**: `graphics/` 配下に新規サブフォルダを追加した場合は `GRAPHICS` 行に手動追加が必要。  
> `generate_assets.exe` を使えばフォルダ作成とMakefile更新が自動化される。

---

## 開発ツール (tools/)

### ui_compiler.py (自動実行)
- `ui/screens/*.json` を読み込み `include/ui_data_*.h` を生成
- レイアウトツリーをビルド時に絶対座標にフラット化（GBA最適化）
- 処理の最後に **`generate_audio_assets`** を実行し、`audio_manifest.json` から `audio_ids.h` / `audio_dispatch_*.gen.h` を更新（UI と音声をビルド前に一度に更新）
- `make` 実行時に `EXTTOOL` として自動呼び出し

### generate_assets.py / generate_assets.exe
- `assets_list.json` を読み込みアセット管理を自動化
  - フォルダ作成
  - ダミー画像 (.bmp) 生成  
  - `image_manifest.json` 更新
  - Makefile の `GRAPHICS` 行更新

### Webエディタ (ブラウザで開いて使用)
- **`tools/ui_editor/index.html`**: 画面レイアウトの視覚的編集 → JSON出力
- **`tools/asset_editor/index.html`**: アセット一覧管理 → `assets_list.json` 出力

---

## TODO / 未実装

- [ ] 実際のドット絵素材への差し替え（現在はダミー `.bmp`）
- [ ] `assets_list.json` へのスチル・アイコン全リスト反映
- [ ] UIエディタでのメニュー画面本番レイアウト作成
- [ ] 各 State への `set_slot()` 追加（アクティブスロット切り替え対応）
- [ ] チャプター1以降のストーリースクリプト追加
- [ ] キャラクタースチル (`stills/chara/`) の実装

---

## Google AI Studio で開発を続ける際のヒント

1. **まずこのファイルを読み込ませる**（全体像の即時把握）
2. **`tools/image_manifest.json`** も読み込むと画像セット構造が復元できる
3. **`ui/screens/*.json`** + **`src/state/`** の対象ファイルを読み込むと特定の画面実装に集中できる
4. UIレイアウトの変更は **`ui/screens/` のJSONを編集** → ビルドで自動的にC++ヘッダが更新される（`ui_compiler.py`）
5. 新しいスプライト追加時は `image_manifest.json` と `ui_manager.cpp` のルックアップテーブル両方の更新が必要
