#include "story_state.h"

#include "state/Manager/state_manager.h"
#include "state/shared_context.h"
#include "state/state_id.h"
#include "save/save_data.h"

#include "bn_assert.h"

// =============================================================
// コンストラクタ
// =============================================================
StoryState::StoryState()
    : chapter_idx_(0), step_idx_(0),
      pending_puzzle_(false), pending_puzzle_ref_(0) {
}

// =============================================================
// enter: SaveDataから進捗を読み込み、最初のステップをpush
// =============================================================
void StoryState::enter(StateManager& sm, SharedContext& ctx) {
    BN_ASSERT(ctx.save != nullptr, "StoryState::enter: save is null");

    const SaveSlot& slot = ctx.save->slots[ctx.active_slot];
    chapter_idx_ = static_cast<int>(slot.story_chapter_idx);
    step_idx_    = static_cast<int>(slot.story_step_idx);
    pending_puzzle_     = false;
    pending_puzzle_ref_ = 0;

    // SharedContextに同期
    ctx.story_chapter_idx    = chapter_idx_;
    ctx.story_step_idx       = step_idx_;
    ctx.story_step_completed = false;
    ctx.story_intro_completed = false;

    // 最初のステップを起動
    push_next_step(sm, ctx);
}

// =============================================================
// update: StoryState自体は毎フレームの処理なし
// （子Stateがスタック上にいる間、updateは呼ばれない）
// =============================================================
void StoryState::update(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    // 全ステップが終わった場合などのフォールバック
    // 通常ここには来ない（push_next_stepでMENUへ遷移するため）
}

// =============================================================
// exit: ステートを抜ける時にSaveDataへ書き込む
// =============================================================
void StoryState::exit(StateManager& /*sm*/, SharedContext& ctx) {
    save_progress(ctx);
}

// =============================================================
// pause: 子StateがさらにStateをpushした時（通常使用しない）
// =============================================================
void StoryState::pause(StateManager& /*sm*/, SharedContext& /*ctx*/) {
}

// =============================================================
// resume: 子State（EVENT/STILL_EVENT/PUZZLE）が終了してpopされた時に呼ばれる
// =============================================================
void StoryState::resume(StateManager& sm, SharedContext& ctx) {
    // --- intro_event完了後にPUZZLEをpushする2段階処理 ---
    if (pending_puzzle_) {
        // intro_eventのEVENTStateが終わったので、次はPUZZLE本体へ
        pending_puzzle_      = false;
        ctx.story_step_completed = false;
        ctx.story_intro_completed = true;
        ctx.target_puzzle_level = pending_puzzle_ref_;
        ctx.puzzle_return_state = StateID::STORY;
        sm.push_state(StateID::PUZZLE);
        return;
    }

    // story_step_completedがfalseなら、子StateがStoryモード外で終了したケース
    // （Bボタンでメニューへ戻るなど）→ StoryStateもMENUへ
    if (!ctx.story_step_completed) {
        sm.change_state(StateID::MENU);
        return;
    }

    // ステップ完了 → 次のステップへ
    ctx.story_step_completed  = false;
    ctx.story_intro_completed = false;
    step_idx_++;
    ctx.story_step_idx = step_idx_;

    // 進捗をSaveDataに即時同期（オートセーブ対応）
    save_progress(ctx);

    push_next_step(sm, ctx);
}

// =============================================================
// push_next_step: 現在のstep_idx_に応じて次のStateをpush
// =============================================================
void StoryState::push_next_step(StateManager& sm, SharedContext& ctx) {
    BN_ASSERT(chapter_idx_ >= 0 && chapter_idx_ < kStoryChapterCount,
              "StoryState: chapter_idx out of range");

    const FdStoryChapter& ch = g_story_chapters[chapter_idx_];

    if (step_idx_ >= ch.num_steps) {
        // このチャプターのステップを全て終えた
        handle_chapter_clear(sm, ctx);
        return;
    }

    const FdStoryStep& step = ch.steps[step_idx_];

    switch (step.type) {
        case FdStoryStepType::STILL_EVENT:
            BN_ASSERT(step.event_ref != nullptr, "STILL_EVENT: event_ref is null");
            ctx.target_event_id = bn::string_view(step.event_ref);
            ctx.story_step_completed = false;
            ctx.event_return_state = StateID::STORY;
            sm.push_state(StateID::STILL_EVENT);
            break;

        case FdStoryStepType::EVENT:
            BN_ASSERT(step.event_ref != nullptr, "EVENT: event_ref is null");
            ctx.target_event_id = bn::string_view(step.event_ref);
            ctx.story_step_completed = false;
            ctx.event_return_state = StateID::STORY;
            sm.push_state(StateID::EVENT);
            break;

        case FdStoryStepType::PUZZLE:
            if (step.intro_event_ref != nullptr && !ctx.story_intro_completed) {
                // intro_eventがある → まずEVENTをpushし、2段階でPUZZLEへ
                pending_puzzle_     = true;
                pending_puzzle_ref_ = step.puzzle_ref;
                ctx.target_event_id = bn::string_view(step.intro_event_ref);
                ctx.story_step_completed = false;
                ctx.event_return_state = StateID::STORY;
                sm.push_state(StateID::EVENT);
            } else {
                // intro_eventなし、またはintro完了済み → 直接PUZZLE
                ctx.target_puzzle_level  = step.puzzle_ref;
                ctx.puzzle_return_state  = StateID::STORY;
                ctx.story_step_completed = false;
                sm.push_state(StateID::PUZZLE);
            }
            break;
        default:
            break;
    }
}

// =============================================================
// handle_chapter_clear: チャプタークリア処理
// =============================================================
void StoryState::handle_chapter_clear(StateManager& sm, SharedContext& ctx) {
    int next_chapter = chapter_idx_ + 1;

    if (next_chapter < kStoryChapterCount) {
        // 次のチャプターへ
        chapter_idx_ = next_chapter;
        step_idx_    = 0;
        ctx.story_chapter_idx = chapter_idx_;
        ctx.story_step_idx    = step_idx_;
        save_progress(ctx);
        push_next_step(sm, ctx);
    } else {
        // 全チャプタークリア → メニューへ
        // TODO: エンディング演出、クリア保存など
        save_progress(ctx);
        sm.change_state(StateID::MENU);
    }
}

// =============================================================
// save_progress: 進捗をSaveDataに書き込む
// =============================================================
void StoryState::save_progress(SharedContext& ctx) {
    if (!ctx.save) return;
    SaveSlot& slot = ctx.save->slots[ctx.active_slot];
    slot.story_chapter_idx = static_cast<uint8_t>(chapter_idx_);
    slot.story_step_idx    = static_cast<uint8_t>(step_idx_);
    save_slot_save(*ctx.save, ctx.active_slot);
}
