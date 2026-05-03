#ifndef SAVE_SELECT_STATE_H
#define SAVE_SELECT_STATE_H

#include "state/state.h"
#include "save/save_data.h"
#include "bn_sprite_text_generator.h"
#include "bn_optional.h"
#include "../gfx/ui_manager.h"
#include "../gfx/fade_effect.h"

class SaveSelectUI {
public:
    SaveSelectUI(UIManager& ui) : ui_(ui) {}
    void set_slot_text(int index, const bn::string_view& text) {
        if (index == 0) ui_.set_text("slot_0", text);
        else if (index == 1) ui_.set_text("slot_1", text);
        else if (index == 2) ui_.set_text("slot_2", text);
    }
private:
    UIManager& ui_;
};

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
    void update_select(StateManager& manager);
    void update_slots_ui();

    bn::sprite_text_generator& text_gen_;
    SaveData& save_;
    int cursor_;
    int selected_slot_;
    FadeEffect fade_;
    UIManager ui_manager_;
    bn::optional<SaveSelectUI> ui_;
    PhaseStep step_;

    static constexpr int FADE_FRAMES = 30;
};

#endif // SAVE_SELECT_STATE_H
