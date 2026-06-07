#include "fix_data_manager.h"

bn::string_view FixDataManager::find_text(bn::string_view id) const {
    for (uint16_t i = 0; i < kTextCount; ++i) {
        if (id == bn::string_view(g_texts[i].id)) {
            return bn::string_view(g_texts[i].ja);
        }
    }
    return bn::string_view();
}

const FdCharacterEntry* FixDataManager::find_character(bn::string_view id) const {
    for (uint16_t i = 0; i < kCharacterCount; ++i) {
        if (id == bn::string_view(g_characters[i].id)) {
            return &g_characters[i];
        }
    }
    return nullptr;
}

bn::string_view FixDataManager::find_face_resource(bn::string_view chara_id, FdFaceId face_id) const {
    const FdCharacterEntry* chara = find_character(chara_id);
    if (chara && face_id != FdFaceId::None) {
        uint8_t idx = static_cast<uint8_t>(face_id);
        if (idx < kFaceImageCount) {
            const char* img = chara->face_images[idx];
            if (img) {
                return bn::string_view(img);
            }
        }
    }
    return bn::string_view();
}

const FdEventEntry* FixDataManager::find_event(bn::string_view id) const {
    for (uint16_t i = 0; i < kEventCount; ++i) {
        if (id == bn::string_view(g_events[i].id)) {
            return &g_events[i];
        }
    }
    return nullptr;
}

const FdEventEntry* FixDataManager::find_puzzle_event(bn::string_view id) const {
    for (uint16_t i = 0; i < kPuzzleEventCount; ++i) {
        if (id == bn::string_view(g_puzzle_events[i].id)) {
            return &g_puzzle_events[i];
        }
    }
    return nullptr;
}

const FdGalleryEntry* FixDataManager::find_gallery(bn::string_view category, bn::string_view resource_id) const {
    for (uint16_t i = 0; i < kGalleryCount; ++i) {
        if (category == bn::string_view(g_gallery[i].category) && 
            resource_id == bn::string_view(g_gallery[i].resource_id)) {
            return &g_gallery[i];
        }
    }
    return nullptr;
}

bool FixDataManager::apply_unlock_rules(bn::string_view event_id, SaveSlot& slot, SaveData& save, int slot_index) const {
    bool changed = false;
    for (int16_t i = 0; i < kUnlockRuleCount; ++i) {
        const FdUnlockRule& rule = g_unlock_rules[i];
        if (rule.event_id == nullptr) continue;
        if (event_id != bn::string_view(rule.event_id)) continue;
        if (rule.flag_id < 0) continue;

        // フラグが未設定なら立てる
        if (!save_slot_get_flag(slot, rule.flag_id)) {
            save_slot_set_flag(slot, rule.flag_id, true);
            changed = true;
        }
    }
    if (changed) {
        save_slot_save(save, slot_index);
    }
    return changed;
}

bool FixDataManager::toggle_event_unlocked(bn::string_view event_id, SaveSlot& slot, SaveData& save, int slot_index) const {
    bool is_unlocked = is_event_unlocked(event_id, slot);
    bool changed = false;
    for (int16_t i = 0; i < kUnlockRuleCount; ++i) {
        const FdUnlockRule& rule = g_unlock_rules[i];
        if (rule.event_id == nullptr) continue;
        if (event_id != bn::string_view(rule.event_id)) continue;
        if (rule.flag_id < 0) continue;

        save_slot_set_flag(slot, rule.flag_id, !is_unlocked);
        changed = true;
    }
    if (changed) {
        save_slot_save(save, slot_index);
    }
    return changed;
}

bool FixDataManager::is_event_unlocked(bn::string_view event_id, const SaveSlot& slot) const {
    bool has_rule = false;
    for (int16_t i = 0; i < kUnlockRuleCount; ++i) {
        const FdUnlockRule& rule = g_unlock_rules[i];
        if (rule.event_id == nullptr) continue;
        if (event_id != bn::string_view(rule.event_id)) continue;
        if (rule.flag_id < 0) continue;

        has_rule = true;
        if (!save_slot_get_flag(slot, rule.flag_id)) {
            return false;
        }
    }
    return has_rule;
}

void FixDataManager::unlock_all_gallery_item_flags(SaveSlot& slot, SaveData& save, int slot_index) const {
    bool changed = false;
    for (uint16_t i = 0; i < kGalleryCount; ++i) {
        const int16_t f = g_gallery[i].unlock_flag;
        if (f >= 0 && !save_slot_get_flag(slot, f)) {
            save_slot_set_flag(slot, f, true);
            changed = true;
        }
    }
    if (changed) {
        save_slot_save(save, slot_index);
    }
}