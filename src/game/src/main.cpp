#include "bn_core.h"
#include "bn_sprite_text_generator.h"

#include "japanese_sprite_font.h"

#include "state/Manager/state_manager.h"
#include "state/shared_context.h"
#include "state/Title/title_state.h"
#include "state/MainMenu/menu_state.h"
#include "state/MainPuzzle/puzzle_state.h"
#include "state/Setting/settings_state.h"
#include "state/Event/event_state.h"
#include "state/Endless/endless_state.h"
#include "state/Practice/practice_menu_state.h"
#include "state/Gallery/gallery_state.h"
#include "state/Debug/debug_state.h"
#include "audio/sound_manager.h"
#include "save/save_data.h"
#include "game/story_data.h"
#include "input/input_manager.h"

// ゲームフロー列挙子
// TitleState が pop したら SAVE_SELECT へ
// SaveSelectState が pop したら MENU へ
// MENU から各モードへ分岐
enum class GameFlow {
    TITLE,
    MENU,
    STORY_EVENT,
    PUZZLE,
    PRACTICE,
    ENDLESS,
    GALLERY,
    SETTINGS,
    DEBUG,
};

int main()
{
    bn::core::init();

    bn::sprite_text_generator text_generator(japanese_sprite_font);

    SoundManager sound;
    SaveData save;
    StateManager state_manager;

    // SRAM からセーブデータをロード
    save_data_load(save);

    // 全 State を確保
    BN_DATA_EWRAM static TitleState         title_state;
    BN_DATA_EWRAM static MenuState          menu_state;
    BN_DATA_EWRAM static PuzzleState        puzzle_state;
    BN_DATA_EWRAM static SettingsState      settings_state;
    BN_DATA_EWRAM static EventState         event_state;
    BN_DATA_EWRAM static EndlessState       endless_state;
    BN_DATA_EWRAM static PracticeMenuState  practice_menu_state;
    BN_DATA_EWRAM static GalleryState       gallery_state;
    BN_DATA_EWRAM static DebugState         debug_state;

    // State の登録
    state_manager.register_state(StateID::TITLE, &title_state);
    state_manager.register_state(StateID::MENU, &menu_state);
    state_manager.register_state(StateID::PUZZLE, &puzzle_state);
    state_manager.register_state(StateID::SETTINGS, &settings_state);
    state_manager.register_state(StateID::EVENT, &event_state);
    state_manager.register_state(StateID::ENDLESS, &endless_state);
    state_manager.register_state(StateID::PRACTICE, &practice_menu_state);
    state_manager.register_state(StateID::GALLERY, &gallery_state);
    state_manager.register_state(StateID::DEBUG_MENU, &debug_state);

    // 共有コンテキストの初期化
    SharedContext ctx;
    ctx.text_generator = &text_generator;
    ctx.sound = &sound;
    ctx.save = &save;
    ctx.active_slot = 0;
    ctx.story_script_index = 0;

    // 最初の状態をセット
    state_manager.change_state(StateID::TITLE);

    while(true)
    {
        InputManager::instance().update();
        state_manager.update(ctx);
        bn::core::update();
    }
}
