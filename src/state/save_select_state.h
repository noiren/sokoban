#ifndef SAVE_SELECT_STATE_H
#define SAVE_SELECT_STATE_H

#include "state/state.h"
#include "save/save_data.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

// セーブスロット選択画面
// ・スロット1〜3を上下で選択
// ・A: 選択（新規 or ロード）
// ・B: タイトルに戻る（TODO: 今後検討）
class SaveSelectState : public State {
public:
    SaveSelectState(bn::sprite_text_generator& text_gen, SaveData& save);

    void init(StateManager& manager) override;
    void update(StateManager& manager) override;
    void shutdown() override;

    // 選択されたスロットインデックス (0-2)
    int selected_slot() const { return selected_slot_; }

private:
    void draw_slots();

    bn::sprite_text_generator& text_gen_;
    SaveData& save_;
    int cursor_;
    int selected_slot_;
    bn::vector<bn::sprite_ptr, 64> sprites_;
};

#endif // SAVE_SELECT_STATE_H
