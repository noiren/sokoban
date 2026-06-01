#ifndef STORY_STATE_H
#define STORY_STATE_H

#include "state/state.h"
#include "generated/generated_fix_data.h"

// =============================================================
// StoryState
//
// ストーリーモードの進行を管理する「純粋な進行管理State」。
// 自身は画面を持たず、スタックの底に常駐する。
//
// 【動作フロー】
//  enter()   : SaveDataからchapter/step_idxを読み込み、最初のステップをpush
//  resume()  : 子State(EVENT/STILL_EVENT/PUZZLE)が完了して戻ってきたら
//               story_step_completedフラグを確認し、次のステップへ進む
//  exit()    : チャプタークリアやメニューへ戻る時にSaveDataへ書き込む
//
// 【intro_event付きPUZZLEの2段階処理】
//  1. intro_eventをEVENTとしてpush → 完了でresume()
//  2. story_intro_completedがtrueなら、次はPUZZLEをpush
// =============================================================
class StoryState : public State {
public:
    StoryState();

    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override;
    void resume(StateManager& sm, SharedContext& ctx) override;

private:
    // 現在のステップを chapter/step_idx に基づいてpush
    void push_next_step(StateManager& sm, SharedContext& ctx);

    // チャプター末尾に達した時の処理（次チャプターへ or エンディング）
    void handle_chapter_clear(StateManager& sm, SharedContext& ctx);

    // SaveDataに進捗を書き込む
    void save_progress(SharedContext& ctx);

    int chapter_idx_;       // 現在のチャプターインデックス
    int step_idx_;          // チャプター内のステップインデックス
    bool pending_puzzle_;   // intro_event再生後、PUZZLEをpushする待機フラグ
    int  pending_puzzle_ref_; // pending_puzzle_がtrueの時のパズル番号
};

#endif // STORY_STATE_H
