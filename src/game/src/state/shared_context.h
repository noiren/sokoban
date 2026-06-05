#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

#include "bn_string_view.h"
#include "state_id.h"

struct SaveData;
class EffectManager; // 前方宣言
namespace bn { class sprite_text_generator; }

struct SharedContext {
    bn::sprite_text_generator* text_generator = nullptr;
    SaveData* save = nullptr;
    EffectManager* effect_manager = nullptr; // 追加
    
    int active_slot = 0;
    
    // 画面遷移用ペイロード (Payloads for state transitions)
    int target_puzzle_level = 0;
    bn::string_view target_event_id;
    StateID event_return_state = StateID::MENU; // イベント終了後に戻るState
    bool event_is_overlay = false; // パズル上にオーバーレイ表示するかどうか
    bool use_puzzle_event_table = false; // g_puzzle_eventsを使用するかどうか
    StateID puzzle_return_state = StateID::MENU; // パズル終了後に戻るState

    // ストーリー進行ペイロード
    // StoryState がセット・参照する。各子Stateは変更不要。
    int  story_chapter_idx = 0;       // 現在のチャプターインデックス
    int  story_step_idx    = 0;       // チャプター内のステップインデックス

    // ストーリーステップ完了通知
    // 各子State（EventState/PuzzleState/StillEventState）が終了時に true にする。
    // StoryState の resume() で必ず読み取り、false に戻すこと。
    bool story_step_completed = false;

    // StoryState内部: intro_eventの再生が完了したか
    // （PUZZLE型ステップのintro_event→パズル本体の2段階遷移制御用）
    bool story_intro_completed = false;

    // パズルクリア結果（PracticeMenuState の resume() で読み取る）
    // PuzzleState がクリア時にセット、PracticeMenuState が読み取り後に false に戻す
    bool puzzle_just_cleared   = false;
    int  puzzle_clear_level    = 0;   // クリアしたレベル番号
    int  puzzle_clear_moves    = 0;   // クリア時の手数
    int  puzzle_clear_frames   = 0;   // クリア時の経過フレーム数

    // パズルイベントの再生状態 (プレイセッション中のみ維持)
    bool puzzle_played_intro[64] = {false};
    bool puzzle_played_outro[64] = {false};
};

#endif // SHARED_CONTEXT_H