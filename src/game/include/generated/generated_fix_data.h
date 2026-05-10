// AUTO GENERATED FILE. DO NOT EDIT.
#pragma once
#include <cstdint>

enum class FdFaceId : uint8_t {
    Normal_1, Normal_2, Normal_3,
    Smile_1, Smile_2, Smile_3,
    Sad_1, Sad_2, Sad_3,
    Angry_1, Angry_2, Angry_3,
    Surprised_1, Surprised_2, Surprised_3,
    Happy_1, Happy_2, Happy_3,
    Think_1, Think_2, Think_3,
    None = 255
};

enum class FdPosition : uint8_t {
    Left,
    Right
};

struct FdCharacterEntry {
    const char* id;
    const char* name_ja;
    const char* face_images[21];
};

struct FdTextEntry {
    const char* id;
    const char* category;
    const char* ja;
};

struct FdEventLine {
    const char* speaker_id;
    FdFaceId face_id;
    FdPosition position;
    const char* image_id;
    const char* text;
};

struct FdEventEntry {
    const char* id;
    const char* title_ja;
    const FdEventLine* lines;
    uint16_t line_count;
};

struct FdGalleryEntry {
    const char* category;
    const char* resource_id;
    const char* ja;
};

static constexpr FdCharacterEntry g_characters[] = {
    {"chara_mayo", "マヨ", {"mayo_normal", "mayo_happy1", "mayo_happy2", "mayo_happy3", "mayo_sad", "mayo_angry", "mayo_surprised", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {"chara_b", "キャラB", {"chara_b_normal", "chara_b_happy1", "chara_b_sad", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
};
static constexpr uint16_t kCharacterCount = 2;

static constexpr FdTextEntry g_texts[] = {
    {"MSG_PRESS_START", "system", "スタートを押してください"},
    {"MSG_SAVING", "system", "セーブ中..."},
    {"MSG_SAVE_COMPLETE", "system", "セーブしました"},
    {"MSG_LOAD_FAILED", "system", "データを読み込めませんでした"},
    {"MSG_NO_DATA", "system", "データがありません"},
    {"MSG_NEW_GAME", "system", "はじめから"},
    {"MSG_CONTINUE", "system", "つづきから"},
    {"MSG_YES", "system", "はい"},
    {"MSG_NO", "system", "いいえ"},
    {"MSG_BACK", "system", "もどる"},
    {"MSG_CONFIRM", "system", "決定"},
    {"MSG_CANCEL", "system", "キャンセル"},
    {"UI_TITLE_MENU_STORY", "ui", "ストーリー"},
    {"UI_TITLE_MENU_ENDLESS", "ui", "エンドレス"},
    {"UI_TITLE_MENU_PRACTICE", "ui", "れんしゅう"},
    {"UI_TITLE_MENU_GALLERY", "ui", "ギャラリー"},
    {"UI_TITLE_MENU_SETTINGS", "ui", "せってい"},
    {"UI_SETTINGS_BGM", "ui", "BGM"},
    {"UI_SETTINGS_SE", "ui", "SE"},
    {"UI_SETTINGS_ON", "ui", "ON"},
    {"UI_SETTINGS_OFF", "ui", "OFF"},
    {"UI_GALLERY_TACHIE", "ui", "立ち絵"},
    {"UI_GALLERY_STILL", "ui", "スチル"},
    {"UI_GALLERY_BGM", "ui", "BGM"},
    {"UI_GALLERY_SE", "ui", "SE"},
    {"UI_PUZZLE_MOVES", "ui", "てすう"},
    {"UI_PUZZLE_RESET", "ui", "リセット"},
    {"UI_PUZZLE_CLEAR", "ui", "クリア！"},
};
static constexpr uint16_t kTextCount = 28;

static constexpr FdEventLine lines_EVT_CH1_CLEAR[] = {
    {"chara_b", FdFaceId::None, FdPosition::Left, "chara_b_happy1", "よくできました！"},
    {"chara_b", FdFaceId::None, FdPosition::Left, "chara_b_happy1", "いいスタートだね。"},
    {"chara_b", FdFaceId::None, FdPosition::Left, "chara_b_normal", "明日はもっとあるから。"},
};
static constexpr FdEventLine lines_EVT_CH1_INTRO[] = {
    {"chara_mayo", FdFaceId::None, FdPosition::Left, "mayo_normal", "また来たんだ…"},
    {"chara_mayo", FdFaceId::None, FdPosition::Left, "mayo_normal", "倉庫か。"},
    {"chara_b", FdFaceId::None, FdPosition::Left, "chara_b_happy1", "来た来た！"},
    {"chara_b", FdFaceId::None, FdPosition::Left, "chara_b_happy1", "荷物の整理、手伝ってよ。"},
    {"chara_b", FdFaceId::None, FdPosition::Left, "chara_b_normal", "準備はいい？"},
};
static constexpr FdEventEntry g_events[] = {
    {"EVT_CH1_CLEAR", "名称未設定", lines_EVT_CH1_CLEAR, 3},
    {"EVT_CH1_INTRO", "名称未設定", lines_EVT_CH1_INTRO, 5},
};
static constexpr uint16_t kEventCount = 2;

static constexpr FdGalleryEntry g_gallery[] = {
    {"tachi-e", "mayo_normal", "マヨ（通常）"},
    {"tachi-e", "mayo_happy1", "マヨ（喜び1）"},
    {"tachi-e", "mayo_happy2", "マヨ（喜び2）"},
    {"tachi-e", "mayo_happy3", "マヨ（喜び3）"},
    {"tachi-e", "mayo_sad", "マヨ（悲しみ）"},
    {"tachi-e", "mayo_angry", "マヨ（怒り）"},
    {"tachi-e", "mayo_surprised", "マヨ（驚き）"},
    {"tachi-e", "chara_b_normal", "キャラB（通常）"},
    {"tachi-e", "chara_b_happy1", "キャラB（喜び）"},
    {"tachi-e", "chara_b_sad", "キャラB（悲しみ）"},
    {"bgm", "Afterburner", "afterburner"},
    {"bgm", "RollinDownTheStreet", "rollin down the street"},
    {"bgm", "FlowerGuysPoolParty", "flowerguy pool party"},
    {"se", "Move", "カーソル移動"},
    {"se", "Push", "ブロック押し"},
    {"se", "Clear", "ステージクリア"},
    {"se", "Reset", "リセット"},
};
static constexpr uint16_t kGalleryCount = 17;

