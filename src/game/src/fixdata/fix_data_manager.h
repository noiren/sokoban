#pragma once

#include "bn_string_view.h"
#include "generated/generated_fix_data.h"
#include "save/save_data.h"

class FixDataManager {
public:
    static FixDataManager& instance() {
        static FixDataManager inst;
        return inst;
    }

    bn::string_view find_text(bn::string_view id) const;
    const FdCharacterEntry* find_character(bn::string_view id) const;
    bn::string_view find_face_resource(bn::string_view chara_id, FdFaceId face_id) const;
    const FdEventEntry* find_event(bn::string_view id) const;
    const FdEventEntry* find_puzzle_event(bn::string_view id) const;
    const FdGalleryEntry* find_gallery(bn::string_view category, bn::string_view resource_id) const;

    // イベントIDに対応する解禁ルールを適用し、SRAMに保存する
    // event_id: 完了したイベントまたはスチルイベントのID
    bool apply_unlock_rules(bn::string_view event_id, SaveSlot& slot, SaveData& save, int slot_index) const;

    // イベントIDに紐づく全ての解禁フラグをトグルする（立っていれば全てOFF、そうでなければ全てON）
    bool toggle_event_unlocked(bn::string_view event_id, SaveSlot& slot, SaveData& save, int slot_index) const;

    // イベントIDに紐づく全ての解禁フラグが立っているか確認
    bool is_event_unlocked(bn::string_view event_id, const SaveSlot& slot) const;

private:
    FixDataManager() = default;
};