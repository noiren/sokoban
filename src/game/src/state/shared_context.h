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
    StateID puzzle_return_state = StateID::MENU; // パズル終了後に戻るState
};

#endif // SHARED_CONTEXT_H