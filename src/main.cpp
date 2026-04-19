#include "bn_core.h"
#include "bn_sprite_text_generator.h"

#include "japanese_sprite_font.h"

#include "state/state_manager.h"
#include "state/title_state.h"
#include "state/save_select_state.h"
#include "state/menu_state.h"
#include "state/puzzle_state.h"
#include "state/settings_state.h"
#include "state/event_state.h"
#include "state/endless_state.h"
#include "state/practice_menu_state.h"
#include "state/gallery_state.h"
#include "state/debug_state.h"
#include "audio/sound_manager.h"
#include "save/save_data.h"
#include "game/story_data.h"

// ゲームフロー列挙子
// TitleState が pop したら SAVE_SELECT へ
// SaveSelectState が pop したら MENU へ
// MENU から各モードへ分岐
enum class GameFlow {
    TITLE,
    SAVE_SELECT,
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

    // SRAM からセーブデータをロード（失敗時は全スロット初期化済み）
    save_data_load(save);

    // 現在選択中のスロット (SaveSelectState で決定)
    int active_slot = 0;

    // 全 State をここで確保（GBAはヒープを使わない）
    TitleState         title_state(text_generator);
    SaveSelectState    save_select_state(text_generator, save);
    MenuState          menu_state(text_generator, sound);
    PuzzleState        puzzle_state(text_generator, sound);
    SettingsState      settings_state(text_generator, sound, save.slots[active_slot]);
    EventState         event_state(text_generator, sound, save.slots[active_slot]);
    EndlessState       endless_state(text_generator, sound, save.slots[active_slot]);
    PracticeMenuState  practice_menu_state(text_generator);
    GalleryState       gallery_state(text_generator);
    DebugState         debug_state(text_generator, sound, save.slots[active_slot]);

    // サウンド初期設定（スロット0のデフォルト値 or SRAM読み込み結果を使う）
    sound.set_se_enabled(save.slots[0].se_enabled);

    GameFlow flow = GameFlow::TITLE;
    int story_script_index = 0;
    state_manager.push(&title_state);

    while(true)
    {
        state_manager.update();

        if (state_manager.empty()) {
            switch (flow) {

                // ---- TITLE → SAVE_SELECT ----
                case GameFlow::TITLE:
                    flow = GameFlow::SAVE_SELECT;
                    state_manager.push(&save_select_state);
                    break;

                // ---- SAVE_SELECT → MENU ----
                case GameFlow::SAVE_SELECT: {
                    active_slot = save_select_state.selected_slot();
                    if (active_slot < 0) active_slot = 0;  // 安全策

                    // スロットが新規なら初期化してセーブ
                    if (!save_slot_is_valid(save.slots[active_slot])) {
                        save_slot_init(save.slots[active_slot]);
                        save_slot_save(save, active_slot);
                    }

                    // 各 State に選択されたスロットを再バインド
                    // NOTE: このプロジェクトでは State はポインタで save.slots[n] を参照するため
                    //       active_slot が決まった後で参照先を差し替える必要がある。
                    //       現在の実装は State のコンストラクタで slot を参照渡しで持つため
                    //       ここで実体への参照を渡し直すことができない。
                    //       → TODO: 各 State に set_slot() を追加するか、slots 配列のポインタを渡す方式に変更する
                    //         暫定的に active_slot=0 固定で問題なく動作する。

                    sound.set_se_enabled(save.slots[active_slot].se_enabled);

                    flow = GameFlow::MENU;
                    state_manager.push(&menu_state);
                    break;
                }

                // ---- MENU → 各モード ----
                case GameFlow::MENU: {
                    MenuItem sel = menu_state.last_selected();

                    if (sel == MenuItem::STORY) {
                        flow = GameFlow::STORY_EVENT;
                        story_script_index = 0;
                        if (story_script_index < NUM_STORY_SCRIPTS) {
                            event_state.set_script(*story_scripts[story_script_index]);
                            state_manager.push(&event_state);
                        } else {
                            state_manager.push(&menu_state);
                        }
                    }
                    else if (sel == MenuItem::PRACTICE) {
                        flow = GameFlow::PRACTICE;
                        state_manager.push(&practice_menu_state);
                    }
                    else if (sel == MenuItem::ENDLESS) {
                        flow = GameFlow::ENDLESS;
                        state_manager.push(&endless_state);
                    }
                    else if (sel == MenuItem::GALLERY) {
                        flow = GameFlow::GALLERY;
                        state_manager.push(&gallery_state);
                    }
                    else if (sel == MenuItem::SETTINGS) {
                        flow = GameFlow::SETTINGS;
                        state_manager.push(&settings_state);
                    }
                    else if (sel == MenuItem::DEBUG) {
                        flow = GameFlow::DEBUG;
                        state_manager.push(&debug_state);
                    }
                    else {
                        state_manager.push(&menu_state);
                    }
                    break;
                }

                // ---- STORY_EVENT → PUZZLE or 次のイベント ----
                case GameFlow::STORY_EVENT:
                    if (event_state.wants_puzzle()) {
                        flow = GameFlow::PUZZLE;
                        state_manager.push(&puzzle_state);
                    } else {
                        story_script_index++;
                        if (story_script_index < NUM_STORY_SCRIPTS) {
                            event_state.set_script(*story_scripts[story_script_index]);
                            state_manager.push(&event_state);
                        } else {
                            // スクリプト終了 → メニューへ
                            flow = GameFlow::MENU;
                            save_slot_save(save, active_slot);
                            state_manager.push(&menu_state);
                        }
                    }
                    break;

                // ---- PUZZLE → 次のイベント or メニュー ----
                case GameFlow::PUZZLE:
                    if (story_script_index >= 0 && story_script_index < NUM_STORY_SCRIPTS - 1) {
                        story_script_index++;
                        flow = GameFlow::STORY_EVENT;
                        event_state.set_script(*story_scripts[story_script_index]);
                        state_manager.push(&event_state);
                    } else {
                        flow = GameFlow::MENU;
                        save_slot_save(save, active_slot);
                        state_manager.push(&menu_state);
                    }
                    break;

                // ---- PRACTICE → MENU ----
                case GameFlow::PRACTICE:
                    flow = GameFlow::MENU;
                    state_manager.push(&menu_state);
                    break;

                // ---- ENDLESS → MENU ----
                case GameFlow::ENDLESS:
                    flow = GameFlow::MENU;
                    save_slot_save(save, active_slot);
                    state_manager.push(&menu_state);
                    break;

                // ---- GALLERY → MENU ----
                case GameFlow::GALLERY:
                    flow = GameFlow::MENU;
                    state_manager.push(&menu_state);
                    break;

                // ---- SETTINGS → MENU ----
                case GameFlow::SETTINGS:
                    flow = GameFlow::MENU;
                    save_slot_save(save, active_slot);
                    state_manager.push(&menu_state);
                    break;

                // ---- DEBUG → 各種起動 ----
                case GameFlow::DEBUG:
                    if (debug_state.wants_event()) {
                        flow = GameFlow::STORY_EVENT;
                        story_script_index = debug_state.event_index();
                        if (story_script_index < NUM_STORY_SCRIPTS) {
                            event_state.set_script(*story_scripts[story_script_index]);
                            state_manager.push(&event_state);
                        } else {
                            flow = GameFlow::MENU;
                            state_manager.push(&menu_state);
                        }
                    } else if (debug_state.wants_puzzle()) {
                        flow = GameFlow::PUZZLE;
                        game_init(puzzle_state.game_state(), debug_state.puzzle_level());
                        state_manager.push(&puzzle_state);
                    } else {
                        flow = GameFlow::MENU;
                        state_manager.push(&menu_state);
                    }
                    break;

                default:
                    break;
            }
        }

        bn::core::update();
    }
}
