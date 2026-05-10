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
        if (idx < 21) {
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

const FdGalleryEntry* FixDataManager::find_gallery(bn::string_view category, bn::string_view resource_id) const {
    for (uint16_t i = 0; i < kGalleryCount; ++i) {
        if (category == bn::string_view(g_gallery[i].category) && 
            resource_id == bn::string_view(g_gallery[i].resource_id)) {
            return &g_gallery[i];
        }
    }
    return nullptr;
}