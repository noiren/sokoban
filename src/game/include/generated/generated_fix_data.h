// AUTO GENERATED FILE. DO NOT EDIT.
#pragma once
#include <cstdint>
#include "generated/audio_ids.h"

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
    BgmId bgm_id;
    SeId se_id;
    bool stop_bgm;
    const char* emotion_id;
};

struct FdEventEntry {
    const char* id;
    const char* title_ja;
    const FdEventLine* lines;
    uint16_t line_count;
};

struct FdStillEventMessage {
    const char* text;
    SeId se_id;
    BgmId bgm_id;
    bool stop_bgm;
};

struct FdStillEventPage {
    const char* still_image_id;
    uint16_t fade_in_frames;
    uint16_t fade_out_frames;
    const FdStillEventMessage* messages;
    uint16_t message_count;
};

struct FdStillEventEntry {
    const char* id;
    const char* title_ja;
    const FdStillEventPage* pages;
    uint16_t page_count;
};

struct FdGalleryEntry {
    const char* category;
    const char* resource_id;
    const char* ja;
    int16_t     unlock_flag; // -1=常に解禁、>=0=フラグ参照
};

struct FdUnlockRule {
    const char* event_id;   // このイベントIDの再生完了時
    int16_t     flag_id;    // このフラグを立てる
};

enum class FdStoryStepType : uint8_t {
    STILL_EVENT = 0,
    EVENT       = 1,
    PUZZLE      = 2,
};

struct FdStoryStep {
    FdStoryStepType type;
    int16_t         puzzle_ref;      // PUZZLE時のみ使用（-1=無効）
    const char*     event_ref;       // EVENT/STILL_EVENT時のみ使用（nullptr=無効）
    const char*     intro_event_ref; // PUZZLE専用：パズル前イベントID（nullptr=なし）
};

struct FdStoryChapter {
    const char*        id;
    const char*        title_ja;
    const FdStoryStep* steps;
    int16_t            num_steps;
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
    {"chara_b", FdFaceId::Happy_1, FdPosition::Right, "chara_b_happy1", "よくできました！", BgmId::Afterburner, SeId::COUNT, false, ""},
    {"chara_b", FdFaceId::Happy_1, FdPosition::Right, "chara_b_happy1", "いいスタートだね。", BgmId::COUNT, SeId::COUNT, false, ""},
    {"chara_b", FdFaceId::Normal_1, FdPosition::Right, "chara_b_normal", "明日はもっとあるから。", BgmId::COUNT, SeId::COUNT, false, ""},
};
static constexpr FdEventLine lines_EVT_CH1_INTRO[] = {
    {"chara_mayo", FdFaceId::Normal_1, FdPosition::Left, "mayo_normal", "また来たんだ…", BgmId::FlowerGuysPoolParty, SeId::COUNT, false, ""},
    {"chara_mayo", FdFaceId::Normal_1, FdPosition::Left, "mayo_normal", "倉庫か。", BgmId::COUNT, SeId::COUNT, false, ""},
    {"chara_b", FdFaceId::Happy_1, FdPosition::Right, "chara_b_happy1", "来た来た！", BgmId::COUNT, SeId::COUNT, false, ""},
    {"chara_b", FdFaceId::Happy_1, FdPosition::Right, "chara_b_happy1", "荷物の整理、手伝ってよ。", BgmId::COUNT, SeId::COUNT, false, ""},
    {"chara_b", FdFaceId::Normal_1, FdPosition::Right, "chara_b_normal", "準備はいい？", BgmId::COUNT, SeId::COUNT, false, ""},
};
static constexpr FdEventLine lines_EVT_PUZZLE0_INTRO[] = {
    {"", FdFaceId::None, FdPosition::Left, "", "ここから先は岩を動かして進む必要があります。", BgmId::COUNT, SeId::COUNT, false, ""},
    {"", FdFaceId::None, FdPosition::Left, "", "岩は押すことしかできません。", BgmId::COUNT, SeId::COUNT, false, ""},
};
static constexpr FdEventLine lines_STILL_PROLOGUE[] = {
    {"", FdFaceId::None, FdPosition::Left, "", "これは、とある世界のお話。", BgmId::COUNT, SeId::COUNT, false, ""},
    {"", FdFaceId::None, FdPosition::Left, "", "謎のダンジョンに迷い込んだ主人公の運命は…", BgmId::COUNT, SeId::COUNT, false, ""},
};
static constexpr FdEventEntry g_events[] = {
    {"EVT_CH1_CLEAR", "チャプター1クリア", lines_EVT_CH1_CLEAR, 3},
    {"EVT_CH1_INTRO", "プロローグ", lines_EVT_CH1_INTRO, 5},
    {"EVT_PUZZLE0_INTRO", "パズルチュートリアル", lines_EVT_PUZZLE0_INTRO, 2},
    {"STILL_PROLOGUE", "プロローグ", lines_STILL_PROLOGUE, 2},
};
static constexpr uint16_t kEventCount = 4;

static constexpr FdEventEntry g_puzzle_events[] = {
    {nullptr, nullptr, nullptr, 0}
};
static constexpr uint16_t kPuzzleEventCount = 0;

static constexpr FdStillEventMessage msgs_EVENT_STILL_01_p0[] = {
    {"", SeId::COUNT, BgmId::COUNT, false},
};
static constexpr FdStillEventPage pages_EVENT_STILL_01[] = {
    {"stl_logo", 16, 16, msgs_EVENT_STILL_01_p0, 1},
};
static constexpr FdStillEventMessage msgs_sevt_sample_p0[] = {
    {" ", SeId::COUNT, BgmId::Afterburner, false},
    {"最初のテキストです", SeId::COUNT, BgmId::COUNT, false},
    {"次のテキストです", SeId::COUNT, BgmId::FlowerGuysPoolParty, false},
};
static constexpr FdStillEventMessage msgs_sevt_sample_p1[] = {
    {"さらに次のテキスト...", SeId::COUNT, BgmId::COUNT, false},
};
static constexpr FdStillEventPage pages_sevt_sample[] = {
    {"stl_logo", 60, 60, msgs_sevt_sample_p0, 3},
    {"stl_title", 60, 60, msgs_sevt_sample_p1, 1},
};
static constexpr FdStillEventEntry g_still_events[] = {
    {"EVENT_STILL_01", "OPENING", pages_EVENT_STILL_01, 1},
    {"sevt_sample", "サンプル演出", pages_sevt_sample, 2},
};
static constexpr uint16_t kStillEventCount = 2;

static constexpr FdGalleryEntry g_gallery[] = {
    {"tachi-e", "mayo_normal", "マヨ（通常）", -1},
    {"tachi-e", "mayo_happy1", "マヨ（喜び1）", -1},
    {"tachi-e", "mayo_happy2", "マヨ（喜び2）", -1},
    {"tachi-e", "mayo_happy3", "マヨ（喜び3）", -1},
    {"tachi-e", "mayo_sad", "マヨ（悲しみ）", -1},
    {"tachi-e", "mayo_angry", "マヨ（怒り）", -1},
    {"tachi-e", "mayo_surprised", "マヨ（驚き）", -1},
    {"tachi-e", "chara_b_normal", "キャラB（通常）", -1},
    {"tachi-e", "chara_b_happy1", "キャラB（喜び）", -1},
    {"tachi-e", "chara_b_sad", "キャラB（悲しみ）", -1},
    {"bgm", "Afterburner", "afterburner", -1},
    {"bgm", "RollinDownTheStreet", "rollin down the street", -1},
    {"bgm", "FlowerGuysPoolParty", "flowerguy pool party", -1},
    {"se", "Move", "カーソル移動", -1},
    {"se", "Push", "ブロック押し", -1},
    {"se", "Clear", "ステージクリア", -1},
    {"se", "Reset", "リセット", -1},
};
static constexpr uint16_t kGalleryCount = 17;

static constexpr FdUnlockRule g_unlock_rules[] = {
    {"EVT_CH1_CLEAR", 32},
    {"EVT_CH1_INTRO", 33},
    {"EVT_PUZZLE0_INTRO", 34},
    {"STILL_PROLOGUE", 35},
    {"EVENT_STILL_01", 36},
    {"sevt_sample", 37},
};
static constexpr int16_t kUnlockRuleCount = 6;

static constexpr FdStoryStep steps_ch1[] = {
    {FdStoryStepType::STILL_EVENT, -1, "STILL_PROLOGUE", nullptr},
    {FdStoryStepType::EVENT, -1, "EVT_CH1_INTRO", nullptr},
    {FdStoryStepType::PUZZLE, 0, nullptr, "EVT_PUZZLE0_INTRO"},
    {FdStoryStepType::EVENT, -1, "EVT_CH1_CLEAR", nullptr},
};
static constexpr FdStoryChapter g_story_chapters[] = {
    {"CH1", "第1章", steps_ch1, 4},
};
static constexpr int16_t kStoryChapterCount = 1;

