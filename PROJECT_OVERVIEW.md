# GBA Sokoban (gba-sokoban-bn) プロジェクト構成まとめ

> 最終更新: 2026-05-13
> エンジン: [Butano](https://github.com/GValiente/butano) (C++17, GBA向けゲームライブラリ)
> ビルド: GNU Makefile + ARM GCC クロスコンパイラ

---

## ディレクトリ構成

```text
gba-sokoban-bn/
├── Asset/                      # アセット・データ定義（非プログラマー向け作業領域）
│   ├── audio/                  # BGM・SE音声素材 (.wav, .xm, .it 等) および定義
│   ├── fixdata/                # キャラクター・イベント・テキスト等の静的データ (.json)
│   ├── graphics/               # 画像素材 (sprites, stills, csv等)
│   ├── layouts/                # UIレイアウト定義 (.json)
│   ├── tools/                  # データ作成用UI・オーディオ等の各種専用エディタ
│   └── README_UI.md            # アセット・デザインチーム向けワークフローガイド
├── src/                        # プログラムコード領域
│   ├── game/                   # ゲーム本体コード
│   │   ├── include/            # 共有・自動生成ヘッダ (generated 等)
│   │   └── src/                # C++ 実装
│   │       ├── main.cpp        # エントリポイント
│   │       ├── audio/          # サウンド再生管理 (SoundManager)
│   │       ├── fixdata/        # 固定データ管理 (FixDataManager)
│   │       ├── game/           # パズルロジック・イベントスクリプト (Sokoban, Levels等)
│   │       ├── gfx/            # 描画管理
│   │       ├── input/          # 入力管理 (InputManager)
│   │       ├── save/           # セーブデータ (SRAM)
│   │       ├── state/          # 画面状態 (Title, Menu, Puzzle, Settings等)
│   │       └── ui/             # コンポーネント指向 UIシステム (Core, HUD, 各画面用)
│   ├── tools/                  # Python 開発支援ツール・ビルドスクリプト
│   └── vs_project/             # Visual Studio プロジェクトファイル
├── Makefile                    # ビルド設定
├── CLAUDE.md                   # AIコーディングルール・アーキテクチャ設計書
└── PROJECT_OVERVIEW.md         # 本ファイル
```

---

## アーキテクチャ概要

### 1. ゲームフロー (src/game/src/main.cpp)

スタックベースの `StateManager` で画面遷移を管理。  
各画面は `src/game/src/state/` 配下に格納されています。

```text
TITLE → SAVE_SELECT → MENU ─┬─ STORY → STORY_EVENT ↔ PUZZLE
                              ├─ PRACTICE → (練習レベル選択)
                              ├─ ENDLESS → (無限生成パズル)
                              ├─ GALLERY → (スチル閲覧)
                              ├─ SETTINGS → (設定)
                              └─ DEBUG → (開発者メニュー)
```

GBAはヒープ制限が厳しいため、各種ManagerクラスやStateオブジェクト等は極力スタックまたはグローバルに配置されます。

### 2. UIシステム (コンポーネント指向・データ駆動)

新しいUIアーキテクチャは `src/game/src/ui/Core/` をベースに構築されています。
*   **UIレイアウト**: `Asset/layouts/*.json` をエディタ (`Asset/tools/ui_editor/`) で編集し、ビルド時に自動生成。
*   **コンポーネント管理**: `ui_manager` を中心に、`ui_node`, `ui_image`, `ui_text` 等の階層化されたコンポーネントツリーで画面を構築。
*   **アニメーション**: `ui_anim` クラスを用いて、イージング付きのアニメーション（移動・フェード・拡大縮小など）をサポート。

### 3. データ・入力管理

*   **入力管理 (InputManager)**: `bn::keypad` を直接叩かず、論理アクション (Decide, Cancel, MoveUp 等) やリピート制御を集中管理 (`src/game/src/input/`)。
*   **固定データ (FixDataManager)**: キャラクター、イベント進行、テキストなどのマスターデータを `Asset/fixdata/*.json` から読み込み、ビルド時に自動で C++ ヘッダ化 (`src/game/include/generated/generated_fix_data.h`)。
*   **セーブデータ (SaveData)**: GBA SRAMに保存。スロット切り替え対応 (`src/game/src/save/`)。

---

## ビルド・アセット管理フロー

ビルド実行時 (`make` または `build.bat`) に、`src/tools/prebuild.py` 経由で以下の処理が自動的に走ります：

1. **`ui_compiler.py`**: UIレイアウト `Asset/layouts/*.json` を元に `src/game/include/ui_data_*.h` を自動生成。
2. **`generate_audio_assets.py`**: 音声マニフェストから `src/game/include/generated/audio_ids.h` などを自動生成。
3. **`generate_fix_data.py`**: `Asset/fixdata/*.json` 以下のマスターデータから `src/game/include/generated/generated_fix_data.h` を自動生成。

> **注意**: C++コードから画像や音声などの各種アセット・データを変更したい場合は、**コードを直接書き換えるのではなく**、`Asset/` 以下のファイルやツール経由でJSONを編集し、ビルドを通す運用になっています。

---

## Google AI Studio (LLM) で開発を続ける際のヒント

1. **まずこのファイルと `CLAUDE.md` を読み込ませる**（全体像と設計思想の即時把握）
2. **`src/game/src/state/state.h` や `src/game/src/ui/Core/` を読み込む**と、現在のモダンなオブジェクト・コンポーネント構成が把握できます。
3. UIレイアウトの変更はプログラム側から行うのではなく、**`Asset/layouts/*.json` をエディタで編集**するか、JSONを直接更新します（ビルド時に C++ ヘッダへ変換されます）。
4. 新しい画面の追加や改修時は、`CLAUDE.md` に記載されている **テーブル駆動アーキテクチャ (Phase/Step管理)** に必ず従ってください。
