#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

struct SaveData;
class EffectManager; // 前方宣言
namespace bn { class sprite_text_generator; }

struct SharedContext {
    bn::sprite_text_generator* text_generator = nullptr;
    SaveData* save = nullptr;
    EffectManager* effect_manager = nullptr; // 追加
    
    int active_slot = 0;
};

#endif // SHARED_CONTEXT_H