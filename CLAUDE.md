# GBA Sokoban (Butano C++ Engine) - AI Coding Rules

## 0. AIの基本ルール (Behavioral Rules)
- **勝手に実装しない:** ユーザーからの依頼に対して、いきなりコードを書いて実装を進めるのは厳禁です。
- **事前確認の徹底:** 必ず「こういう設計・対応方針で進めますがOKですか？」と事前に確認し、ユーザーの合意を得てから作業に入ってください。
- **明示的な指示がある場合:** ユーザーから「実装して！」と明確に指示された場合のみ、そのまま実装に移ってください。

## 1. ターゲット環境
- Nintendo Game Boy Advance (ARM7TDMI)
- C++17 / Butano Engine (https://github.com/GValiente/butano)

## 2. 絶対厳守のハードウェア＆エンジン制約 (CRITICAL)
- **リソース解放:** Butanoリソース(`bn::regular_bg_ptr`等)は、Stateの `exit()` で必ず解放すること。`bn::optional<>` を使いReset可能にするのが定石。
- **メモリ管理:** VRAM制限・スタックサイズが極めて小さい。ローカル配列は避け、`new/delete` の代わりに `bn::` コンテナを使用すること。
- **テキスト描画:** `bn::sprite_text_generator` を使用し、毎フレームの再生成は避ける（変化があった時のみ）。
- **入力:** `bn::keypad::a_pressed()` など `_pressed()` は1フレームのみtrueになる仕様に注意。
- **アサートと境界チェック (必須):** ヌルポインタアクセスや配列外参照は絶対に避けること。アクセス前に必ず事前の範囲チェック（またはnullptrチェック）を行い、不正な状態になり得る箇所には `BN_ASSERT(条件, "エラーメッセージ");` を用いてエラー画面を明示的に表示させること。

## 3. アーキテクチャの前提
- **State管理:** `StateManager` スタック (bn::vector<State*,8>) で制御。State基底は `enter()`, `update()`, `exit()` などのライフサイクル関数を持つ。
- **データセーブ:** SRAM（32KB）を使用。`SaveData` 構造体への書き込みは寿命を考慮し最小限に留めること。

## 4. ビルド・開発コマンド
- **標準ビルド:** `make` または `build.bat` (Windows環境用)
  - `build.bat` は `make` を実行した後、ビルド成功時にROM（`gba-sokoban-bn.gba`）を `binary/` フォルダへコピーします。
- **クリーン:** `make clean`
- **リビルド:** `make clean && make`
- **Cursor/VS Codeのタスク:** `.vscode/tasks.json` にて定義されています。
  - `Ctrl+Shift+B` でデフォルトのビルドタスク（`build (make)`）が走ります。
  - `build (ROM copy)` タスクを選ぶと `build.bat` が実行されます。


# ゲーム状態管理（State Management）アーキテクチャ ガイド

本ドキュメントは、当プロジェクトにおける**画面遷移（State）**と**画面内部の進行（Phase）**の設計思想および実装方法を解説したものです。

GBA（Butano）環境の厳しい制約（「動的メモリ確保（`new`）を避ける」「処理落ちを防ぐ」）をクリアしつつ、コードの可読性と拡張性を極限まで高めた設計になっています。
新しく画面を追加する際や、画面内のUI演出を実装する際は、本ドキュメントの設計ルールに従ってください。

---

## 1. 全体像と用語の定義

本プロジェクトでは、状態遷移を**「大枠の画面切り替え（State）」**と**「画面内の進行（Phase）」**の2つの階層に分けて管理しています。

1. **State（ステート）**
   * 「タイトル画面」「メニュー画面」「パズル画面」など、**全く異なる画面**の単位です。
   * **`StateManager`** というクラスが一元管理します。
2. **Phase（フェーズ）**
   * 一つの画面（State）の中での**進行状態**の単位です。
   * 例えばタイトル画面の中の「ロゴ表示」→「注意書き」→「タイトル表示」といった遷移です。
   * 各Stateクラスが**自分自身（内部のテーブル駆動型ステートマシン）**で管理します。

---

## 2. StateManager の仕組み

### 2.1. 画面遷移の仕組み（遅延評価）
`StateManager` は、スタック（積み上げ）構造で現在の画面を保持しています。
画面遷移の指示（例：`sm.change_state()`）が出されたとき、**その場ですぐには遷移しません**。遷移フラグだけを立てておき、**次フレームの先頭**で実際の画面切り替え（現在の画面の破棄と、次の画面の初期化）を行います。

これにより、「画面更新の途中で自分自身のインスタンスが破棄されてゲームがクラッシュする」というバグを完全に防いでいます。

### 2.2. State のライフサイクル関数
各Stateは、以下の5つのライフサイクル関数を持ちます。

* **`enter()`**: 画面が開始された時に1回だけ呼ばれる（画像のロードや初期化）
* **`update()`**: 毎フレーム呼ばれる（入力処理やアニメーションの更新）
* **`exit()`**: 画面が終了して破棄される時に1回だけ呼ばれる（リソースの解放）
* **`pause()`**: 上に別の画面が被さってきた時（Push）に呼ばれる
* **`resume()`**: 上の画面が消えて、再び自分が一番上になった時（Pop）に呼ばれる

### 2.3. 3つの遷移メソッドの使い分け
画面を切り替える時は、`update()` の引数で渡される `StateManager& sm` を使います。

1. **`sm.change_state(StateID::MENU)`**
   * 現在の画面を完全に終了し、次の画面へ行きます。（例：タイトル → メニュー）
   * 呼ばれる関数：`現在の画面の exit()` → `次の画面の enter()`
2. **`sm.push_state(StateID::SETTINGS)`**
   * 現在の画面の状態を維持したまま、上に新しい画面を乗せます。（例：メニュー → 設定画面）
   * 呼ばれる関数：`現在の画面の pause()` → `次の画面の enter()`
3. **`sm.pop_state()`**
   * 一番上に乗っている画面を終了し、下にあった画面を再開します。（例：設定画面 → メニューに戻る）
   * 呼ばれる関数：`現在の画面の exit()` → `下の画面の resume()`

---

## 3. 新しい画面（State）の追加手順

新しく「リザルト画面（ResultState）」を作る場合の手順です。

#### STEP 1: StateID の追加
`state_id.h` の `enum class StateID` に新しいIDを追加します。必ず `COUNT` より上に書いてください。
```cpp
enum class StateID {
    // ...
    RESULT, // 追加
    COUNT
};
```

#### STEP 2: State クラスの作成
`State` クラスを継承し、`enter`, `update`, `exit` などの必須関数をオーバーライドしたクラスを作ります。
```cpp
#include "state.h"

class ResultState : public State {
public:
    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
};
```

#### STEP 3: main.cpp での実体化と登録
GBAではヒープ確保（`new`）をしないため、`main()` の先頭（スタック領域）で実体を宣言し、`StateManager` に登録します。
```cpp
int main() {
    // ...
    StateManager state_manager;
    
    // 実体を宣言
    ResultState result_state;

    // 登録
    state_manager.register_state(StateID::RESULT, &result_state);
    // ...
}
```

---

## 4. State内部のフェーズ管理（テーブル駆動アーキテクチャ）

1つの画面（State）の中で「ロゴ表示 → タイトル表示」のような遷移を行う場合、`update()` の中に巨大な `switch` 文を書くのは**厳禁**です。
本プロジェクトでは、**「関数ポインタのテーブル」を用いたルーティング設計**を採用しています。

### 4.1. テーブル駆動の仕組み
各フェーズ（状態）に対して、それぞれ専用の `enter_xxx` / `update_xxx` / `exit_xxx` という3つの関数を作ります。これを配列（テーブル）に登録しておくことで、`switch` 文を使わずに「現在のフェーズの関数」を直接呼び出します。

#### 実装のテンプレート（例）
```cpp
// 1. フェーズの定義（最後はCOUNTにする）
enum class Phase { LOGO, TITLE, COUNT };

// 2. テーブルの実体定義（cppファイル）
const ResultState::PhaseHandlers ResultState::phase_table_[] = {
    // LOGOフェーズ用
    { &ResultState::enter_logo, &ResultState::update_logo, &ResultState::exit_logo },
    // TITLEフェーズ用
    { &ResultState::enter_title, &ResultState::update_title, &ResultState::exit_title }
};

// 3. update関数の実装（これだけで済む！）
void ResultState::update(StateManager& sm, SharedContext& ctx) {
    // 現在のフェーズの update_xxx() が自動的に呼ばれる
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }
}
```

### 4.2. フェーズ間の遷移方法
フェーズを移行したい場合（例：ロゴが終わったのでタイトルへ）、**必ず `change_phase()` 関数を使います。**

```cpp
void ResultState::change_phase(Phase next) {
    // 1. 現在のフェーズの退場処理（exit_xxx）を呼ぶ
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    // 2. フェーズを切り替える
    phase_ = next;

    // 3. 新しいフェーズの入場処理（enter_xxx）を呼ぶ
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}
```

### 4.3. 内部フェーズ実装のルール
1. **ロードや初期化は `enter_xxx()` に書く**
   画像やテキストのロード、フェードインの開始処理などは `update` の先頭ではなく、必ず `enter_xxx()` の中に書いてください。
2. **クリーンアップは `exit_xxx()` に書く**
   そのフェーズ専用の変数のリセットや、フェーズが終わる時に片付けたい処理は `exit_xxx()` に書きます。
3. **`update_xxx()` は判定と進行だけを行う**
   「Aボタンが押されたか」「アニメーションが終わったか」を判定し、条件を満たしたら `change_phase(次のフェーズ)` を呼ぶだけにとどめます。

### 4.4. フェーズ内のさらなる微細な進行（Step）
フェーズの中でさらに「フェードイン中(OPENING)」「待機中(RUNNING)」「フェードアウト中(CLOSING)」を分けたい場合は、`update_xxx()` の中に限って `switch (step_)` を使用して構いません。

```cpp
void ResultState::update_logo(StateManager& sm, SharedContext& ctx) {
    switch (step_) {
        case PhaseStep::OPENING:
            if (!fade_.update()) step_ = PhaseStep::RUNNING;
            break;
        case PhaseStep::RUNNING:
            if (bn::keypad::a_pressed()) {
                fade_.start_fade_out(30);
                step_ = PhaseStep::CLOSING;
            }
            break;
        case PhaseStep::CLOSING:
            if (!fade_.update()) {
                // フェードアウトが終わったら次のフェーズへ！
                change_phase(Phase::TITLE);
            }
            break;
    }
}
```

---

## 5. 設計の意図とメリット（なぜこうなっているか）

* **カプセル化と凝集度向上**
  「ロゴ画面に関する処理」を修正したい時、`enter_logo`, `update_logo`, `exit_logo` の3関数を見るだけで完全に完結します。他の画面のコードが目に入らないため、バグを埋め込む確率が激減します。
* **CPU負荷の削減（switch撲滅）**
  GBAのCPUにおいて、巨大な `switch` 文は分岐予測の観点から不利になる場合があります。関数ポインタの配列参照による遷移は、処理速度が安定しており非常に高速です。
* **安全なリソース管理**
  `change_phase()` や `StateManager` の機構により、「必ず `exit` が呼ばれてから次の `enter` が呼ばれる」ことがシステムレベルで保証されています。これによりメモリリークや初期化忘れを防いでいます。

新しく機能を追加するAIや開発者は、この**「状態に紐づく enter / update / exit を常にセットで考える」**という思想をベースにコーディングを行ってください。