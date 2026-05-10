#pragma once

#include "bn_string_view.h"
#include "generated/generated_fix_data.h"

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
    const FdGalleryEntry* find_gallery(bn::string_view category, bn::string_view resource_id) const;

private:
    FixDataManager() = default;
};