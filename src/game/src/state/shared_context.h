#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

// ※ 前方宣言。実際のプロジェクトのクラス名に合わせてください
class SoundManager;
struct SaveData;
namespace bn { class sprite_text_generator; }

struct SharedContext {
    bn::sprite_text_generator* text_generator = nullptr;
    SoundManager* sound = nullptr;
    SaveData* save = nullptr;
    
    int active_slot = 0;
    int story_script_index = 0;
};

#endif // SHARED_CONTEXT_H