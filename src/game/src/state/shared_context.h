#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

struct SaveData;
namespace bn { class sprite_text_generator; }

struct SharedContext {
    bn::sprite_text_generator* text_generator = nullptr;
    SaveData* save = nullptr;
    
    int active_slot = 0;
};

#endif // SHARED_CONTEXT_H