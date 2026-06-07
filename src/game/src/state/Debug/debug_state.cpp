#include "debug_state.h"
#include "game/levels.h"

#include "state/Manager/state_manager.h"
#include "input/input_manager.h"
#include "audio/sound_manager.h"
#include "generated/audio_dispatch_bgm.gen.h"
#include "generated/audio_dispatch_se.gen.h"
#include "ui/Core/Effects/fade_effect.h"
#include "ui/Core/Components/effect_manager.h"
#include "ui/Core/Components/ui_image.h"
#include "animation/sprite_anim_manager.h"
#include "bn_sprite_items_spr_dummy.h"
#include "generated/generated_fix_data.h"
#include "fixdata/fix_data_manager.h"
#include "save/user_data_puzzle.h"

#include "bn_backdrop.h"
#include "bn_color.h"
#include "bn_music.h"
#include "bn_music_item.h"
#include "bn_sprite_items_japanese_font.h"
#include "bn_sprite_palettes.h"
#include "bn_string.h"
#include "bn_assert.h"

#include "generated/chara_portraits.gen.h"
#include "ui_data_logo.h"
#include "ui_data_attention.h"
#include "ui_data_autosave_attension.h"
#include "ui_data_title.h"
#include "ui_data_menu.h"
#include "ui_data_mainmenu.h"
#include "ui_data_save_select.h"
#include "ui_data_settings.h"
#include "ui_data_practice_stageSelect.h"
#include "ui_data_sokoban_main.h"
#include "ui_data_event.h"
#include "ui_data_gallerymenu.h"
#include "ui_data_gallery_selectevent.h"
#include "ui_data_gallery_bgm.h"
#include "ui_data_gallery_viewstill.h"
#include "ui_data_gallery_viewbustup.h"

namespace {

static constexpr ui_types::AnimKeyframe test_keyframes[] = {
    { 0,   0.0f,  40.0f,   0.0f, 0.2f, ui_types::EaseType::BOUNCE_OUT },
    { 30,  0.0f,  -20.0f,  180.0f, 1.8f, ui_types::EaseType::EASE_OUT },
    { 60,  0.0f,   0.0f,  360.0f, 1.0f, ui_types::EaseType::LINEAR }
};
static constexpr ui_types::AnimPreset test_preset = {
    "test_eff",
    60,
    ui_types::EaseType::LINEAR,
    3,
    test_keyframes
};

struct UiDebugScreenEntry {
    const char* id;
    const ui_types::ScreenData* screen;
};

// ui_compiler が出力する全 ui_data_* レイアウト（追加したらここにも1行足す）
constexpr UiDebugScreenEntry UI_DEBUG_SCREENS[] = {
    { "logo",                   &ui_data_logo::SCREEN },
    { "attention",              &ui_data_attention::SCREEN },
    { "autosave_attension",     &ui_data_autosave_attension::SCREEN },
    { "title",                  &ui_data_title::SCREEN },
    { "menu",                   &ui_data_menu::SCREEN },
    { "mainmenu",               &ui_data_mainmenu::SCREEN },
    { "save_select",            &ui_data_save_select::SCREEN },
    { "settings",               &ui_data_settings::SCREEN },
    { "practice_stageSelect",   &ui_data_practice_stageSelect::SCREEN },
    { "sokoban_main",           &ui_data_sokoban_main::SCREEN },
    { "event",                  &ui_data_event::SCREEN },
    { "gallerymenu",            &ui_data_gallerymenu::SCREEN },
    { "gallery_selectevent",    &ui_data_gallery_selectevent::SCREEN },
    { "gallery_bgm",            &ui_data_gallery_bgm::SCREEN },
    { "gallery_viewstill",      &ui_data_gallery_viewstill::SCREEN },
    { "gallery_viewbustup",     &ui_data_gallery_viewbustup::SCREEN },
};

constexpr int UI_DEBUG_SCREEN_COUNT = int(sizeof(UI_DEBUG_SCREENS) / sizeof(UI_DEBUG_SCREENS[0]));

} // namespace

DebugState::DebugState()
    : screen_(DebugScreen::Root),
      cursor_(0),
      event_cursor_(0),
      progress_cursor_(0),
      progress_sub_(0),
      test_bgm_id_(BgmId::Afterburner),
      test_se_id_(SeId::Default_Move),
      last_drawn_bgm_playing_(false),
      prev_sound_bgm_enabled_(true),
      prev_sound_se_enabled_(true),
      prev_sprite_intensity_(0),
      had_saved_sprite_intensity_(false),
      step_(PhaseStep::RUNNING) {
}

void DebugState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    screen_ = DebugScreen::Root;
    cursor_ = 0;
    ui_debug_cursor_ = 0;
    ui_portrait_idx_ = 0;
    last_drawn_bgm_playing_ = false;

    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
    portrait_solo_sprite_.reset();

    SoundManager& snd = SoundManager::instance();
    prev_sound_bgm_enabled_ = snd.bgm_enabled();
    prev_sound_se_enabled_ = snd.se_enabled();
    snd.set_bgm_enabled(true);
    snd.set_se_enabled(true);

    bn::backdrop::set_color(bn::color(0, 0, 0));
    FadeEffect::reset_palette();

    prev_sprite_intensity_ = bn::sprite_palettes::intensity();
    had_saved_sprite_intensity_ = true;
    // intensity は「パレットへの効果量」で 1=最大。1 にすると色が白飛びするため 0=通常表示。
    bn::sprite_palettes::set_intensity(bn::fixed(0));

    if (ctx.text_generator) {
        ctx.text_generator->set_palette_item(bn::sprite_items::japanese_font.palette_item());
    }

    sprites_.clear();
    redraw(ctx);
    step_ = PhaseStep::RUNNING;
}

void DebugState::update(StateManager& sm, SharedContext& ctx) {
    if (step_ == PhaseStep::OPENING) {
        step_ = PhaseStep::RUNNING;
    }

    if (step_ != PhaseStep::RUNNING) {
        return;
    }

    switch (screen_) {
        case DebugScreen::Root:
            update_root(sm, ctx);
            break;
        case DebugScreen::UiList:
            update_ui_list(sm, ctx);
            break;
        case DebugScreen::PortraitSolo:
            update_portrait_solo(sm, ctx);
            break;
        case DebugScreen::BgmList:
            update_bgm_list(sm, ctx);
            break;
        case DebugScreen::BgmTest:
            update_bgm_test(sm, ctx);
            break;
        case DebugScreen::SeList:
            update_se_list(sm, ctx);
            break;
        case DebugScreen::SeTest:
            update_se_test(sm, ctx);
            break;
        case DebugScreen::EffectTest:
            update_effect_test(sm, ctx);
            break;
        case DebugScreen::EventTest:
            update_event_test(sm, ctx);
            break;
        case DebugScreen::AnimTest:
            update_anim_test(sm, ctx);
            break;
        case DebugScreen::StillEventTest:
            update_still_event_test(sm, ctx);
            break;
        case DebugScreen::StageList:
            update_stage_list(sm, ctx);
            break;
        case DebugScreen::ProgressData:
            update_progress_data(sm, ctx);
            break;
        default:
            break;
    }

    // SpriteAnimManager を毎フレーム更新
    SpriteAnimManager::instance().update();
}

void DebugState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) {
        return;
    }
    sprites_.clear();
    switch (screen_) {
        case DebugScreen::Root:
            draw_root(ctx);
            break;
        case DebugScreen::UiList:
            draw_ui_list(ctx);
            break;
        case DebugScreen::PortraitSolo:
            draw_portrait_solo(ctx);
            break;
        case DebugScreen::BgmList:
            draw_bgm_list(ctx);
            break;
        case DebugScreen::BgmTest:
            draw_bgm_test(ctx);
            break;
        case DebugScreen::SeList:
            draw_se_list(ctx);
            break;
        case DebugScreen::SeTest:
            draw_se_test(ctx);
            break;
        case DebugScreen::EffectTest:
            draw_effect_test(ctx);
            break;
        case DebugScreen::EventTest:
            draw_event_test(ctx);
            break;
        case DebugScreen::AnimTest:
            draw_anim_test(ctx);
            break;
        case DebugScreen::StillEventTest:
            draw_still_event_test(ctx);
            break;
        case DebugScreen::StageList:
            draw_stage_list(ctx);
            break;
        case DebugScreen::ProgressData:
            draw_progress_data(ctx);
            break;
        default:
            break;
    }
}

void DebugState::update_root(StateManager& sm, SharedContext& ctx) {
    constexpr int lines = 10;
    auto& inp = InputManager::instance();
    bool changed = false;

    if (inp.is_repeat(Action::MoveUp)) {
        cursor_--;
        if (cursor_ < 0) { cursor_ = lines - 1; }
        changed = true;
        SoundManager::instance().play_move();
    }
    if (inp.is_repeat(Action::MoveDown)) {
        cursor_++;
        if (cursor_ >= lines) { cursor_ = 0; }
        changed = true;
        SoundManager::instance().play_move();
    }

    if (inp.is_triggered(Action::Decide)) {
        if (cursor_ == 0) {
            if (ctx.text_generator) {
                screen_ = DebugScreen::UiList;
                ui_debug_cursor_ = 0;
                changed = true;
                if (!ui_manager_) {
                    ui_manager_.emplace(*ctx.text_generator);
                }
                _ui_debug_reload(ctx);
            }
        }
        else if (cursor_ == 1) { screen_ = DebugScreen::BgmList;       cursor_ = 0; changed = true; }
        else if (cursor_ == 2) { screen_ = DebugScreen::SeList;        cursor_ = 0; changed = true; }
        else if (cursor_ == 3) { screen_ = DebugScreen::EffectTest;    cursor_ = 0; changed = true; }
        else if (cursor_ == 4) { screen_ = DebugScreen::EventTest;     event_cursor_ = 0; changed = true; }
        else if (cursor_ == 5) { screen_ = DebugScreen::AnimTest;      event_cursor_ = 0; changed = true; }
        else if (cursor_ == 6) { screen_ = DebugScreen::StillEventTest;event_cursor_ = 0; changed = true; }
        else if (cursor_ == 7) { screen_ = DebugScreen::StageList;     event_cursor_ = 0; changed = true; }
        else if (cursor_ == 8) { screen_ = DebugScreen::ProgressData;  progress_cursor_ = 0; progress_sub_ = 0; changed = true; }
        else if (cursor_ == 9) {
            ctx.debug_gallery_unlock_all = !ctx.debug_gallery_unlock_all;
            changed = true;
            SoundManager::instance().play_move();
        }
    }

    if (inp.is_triggered(Action::Cancel)) {
        sm.change_state(StateID::MENU);
        return;
    }

    if (changed) { redraw(ctx); }
}

void DebugState::draw_root(SharedContext& ctx) {
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -72, "DEBUG", sprites_);

    ctx.text_generator->set_left_alignment();
    const int spacing = 13;
    int y = -52;
    for (int i = 0; i < 10; ++i) {
        bn::string<40> line;
        line.append(cursor_ == i ? ">" : " ");
        switch (i) {
            case 0:  line.append("UI Debug"); break;
            case 1:  line.append("BGM Debug"); break;
            case 2:  line.append("SE Debug"); break;
            case 3:  line.append("Effect Debug"); break;
            case 4:  line.append("Event Debug"); break;
            case 5:  line.append("Anim Debug"); break;
            case 6:  line.append("Still Event Debug"); break;
            case 7:  line.append("Stage Debug"); break;
            case 8:  line.append("Progress Data"); break;
            default:
                line.append("GALLERY unlock ");
                line.append(ctx.debug_gallery_unlock_all ? "[ON]" : "[OFF]");
                break;
        }
        ctx.text_generator->generate(-112, y, line, sprites_);
        y += spacing;
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 72, "U/D A=open B=menu", sprites_);
}

void DebugState::_ui_debug_reload(SharedContext& ctx) {
    BN_ASSERT(ctx.text_generator != nullptr, "DebugState::_ui_debug_reload: text_generator");
    BN_ASSERT(ui_debug_cursor_ >= 0 && ui_debug_cursor_ < UI_DEBUG_SCREEN_COUNT,
              "DebugState::_ui_debug_reload: ui_debug_cursor_");

    if (!ui_manager_) {
        ui_manager_.emplace(*ctx.text_generator);
    }

    ui_manager_->load_screen(*UI_DEBUG_SCREENS[ui_debug_cursor_].screen);
    _ui_debug_apply_event_portraits();
}

void DebugState::_ui_debug_apply_event_portraits() {
    if (!ui_manager_) {
        return;
    }

    if (UI_DEBUG_SCREENS[ui_debug_cursor_].screen != &ui_data_event::SCREEN) {
        return;
    }

    if (chara_portraits::COUNT <= 0) {
        return;
    }

    int idx = ui_portrait_idx_;
    if (idx < 0) {
        idx = 0;
    }
    if (idx >= chara_portraits::COUNT) {
        idx = chara_portraits::COUNT - 1;
    }

    if (UIImage* left = ui_manager_->get_image("char_left")) {
        ui_manager_->change_sprite_image(left, "chara_portraits", idx);
        // イベント枠は 64x64 前提のため、デバッグ時は拡大して確認しやすくする
        left->set_scale(bn::fixed(2));
    }

    if (UIImage* right = ui_manager_->get_image("char_right")) {
        right->set_scale(bn::fixed(2));
    }
}

void DebugState::update_ui_list(StateManager& /*sm*/, SharedContext& ctx) {
    if (ui_manager_) {
        ui_manager_->update();
    }

    auto& inp = InputManager::instance();
    bool changed = false;

    if (inp.is_repeat(Action::MoveUp)) {
        --ui_debug_cursor_;
        if (ui_debug_cursor_ < 0) {
            ui_debug_cursor_ = UI_DEBUG_SCREEN_COUNT - 1;
        }
        _ui_debug_reload(ctx);
        changed = true;
        SoundManager::instance().play_move();
    }

    if (inp.is_repeat(Action::MoveDown)) {
        ++ui_debug_cursor_;
        if (ui_debug_cursor_ >= UI_DEBUG_SCREEN_COUNT) {
            ui_debug_cursor_ = 0;
        }
        _ui_debug_reload(ctx);
        changed = true;
        SoundManager::instance().play_move();
    }

    if (UI_DEBUG_SCREENS[ui_debug_cursor_].screen == &ui_data_event::SCREEN) {
        if (inp.is_repeat(Action::MoveLeft)) {
            --ui_portrait_idx_;
            if (ui_portrait_idx_ < 0) {
                ui_portrait_idx_ = chara_portraits::COUNT - 1;
            }
            _ui_debug_apply_event_portraits();
            changed = true;
            SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveRight)) {
            ++ui_portrait_idx_;
            if (ui_portrait_idx_ >= chara_portraits::COUNT) {
                ui_portrait_idx_ = 0;
            }
            _ui_debug_apply_event_portraits();
            changed = true;
            SoundManager::instance().play_move();
        }
    }

    if (inp.is_triggered(Action::OpenMenu)) {
        if (chara_portraits::COUNT > 0) {
            if (ui_manager_) {
                ui_manager_->clear_all();
                ui_manager_.reset();
            }
            screen_ = DebugScreen::PortraitSolo;
            _portrait_solo_refresh();
            changed = true;
            SoundManager::instance().play_move();
        }
    }

    if (inp.is_triggered(Action::Cancel)) {
        if (ui_manager_) {
            ui_manager_->clear_all();
            ui_manager_.reset();
        }
        screen_ = DebugScreen::Root;
        cursor_ = 0;
        changed = true;
    }

    if (changed) {
        redraw(ctx);
    }
}

void DebugState::_portrait_solo_refresh() {
    portrait_solo_sprite_.reset();
    if (chara_portraits::COUNT <= 0) {
        return;
    }

    int idx = ui_portrait_idx_;
    if (idx < 0) {
        idx = 0;
    }
    if (idx >= chara_portraits::COUNT) {
        idx = chara_portraits::COUNT - 1;
    }

    auto spr = chara_portraits::create_by_index(idx, bn::fixed(0), bn::fixed(-8));
    if (spr) {
        spr->set_scale(bn::fixed(3));
        spr->set_z_order(0);
        portrait_solo_sprite_ = bn::move(*spr);
    }
}

void DebugState::update_portrait_solo(StateManager& /*sm*/, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    bool changed = false;

    if (chara_portraits::COUNT > 0) {
        if (inp.is_repeat(Action::MoveLeft)) {
            --ui_portrait_idx_;
            if (ui_portrait_idx_ < 0) {
                ui_portrait_idx_ = chara_portraits::COUNT - 1;
            }
            _portrait_solo_refresh();
            changed = true;
            SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveRight)) {
            ++ui_portrait_idx_;
            if (ui_portrait_idx_ >= chara_portraits::COUNT) {
                ui_portrait_idx_ = 0;
            }
            _portrait_solo_refresh();
            changed = true;
            SoundManager::instance().play_move();
        }
    }

    if (inp.is_triggered(Action::Cancel)) {
        portrait_solo_sprite_.reset();
        screen_ = DebugScreen::UiList;
        if (ctx.text_generator) {
            if (!ui_manager_) {
                ui_manager_.emplace(*ctx.text_generator);
            }
            _ui_debug_reload(ctx);
        }
        changed = true;
    }

    if (changed) {
        redraw(ctx);
    }
}

void DebugState::draw_portrait_solo(SharedContext& ctx) {
    if (!ctx.text_generator) {
        return;
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -72, "PORTRAIT SOLO", sprites_);

    bn::string<40> line;
    line.append("idx ");
    line.append(bn::to_string<4>(ui_portrait_idx_));
    ctx.text_generator->generate(0, -54, line, sprites_);

    ctx.text_generator->generate(0, 56, "64sq bust x3 (event)", sprites_);
    ctx.text_generator->generate(0, 70, "L/R=B diff B=UI list", sprites_);
}

void DebugState::draw_ui_list(SharedContext& ctx) {
    if (!ctx.text_generator) {
        return;
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "UI DEBUG", sprites_);

    ctx.text_generator->set_left_alignment();
    bn::string<48> line;
    line.append("Layout: ");
    line.append(UI_DEBUG_SCREENS[ui_debug_cursor_].id);
    ctx.text_generator->generate(-112, -52, line, sprites_);

    if (UI_DEBUG_SCREENS[ui_debug_cursor_].screen == &ui_data_event::SCREEN) {
        line.clear();
        line.append("L/R portrait idx ");
        line.append(bn::to_string<4>(ui_portrait_idx_));
        ctx.text_generator->generate(-112, -36, line, sprites_);
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 56, "SELECT=solo (big)", sprites_);
    ctx.text_generator->generate(0, 72, "U/D layout L/R face B=back", sprites_);
}

void DebugState::update_bgm_list(StateManager&, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    const int lines = (BN_GENERATED_BGM_COUNT == 0) ? 1 : static_cast<int>(BN_GENERATED_BGM_COUNT);
    bool changed = false;

    if (lines > 1 || (lines == 1 && BN_GENERATED_BGM_COUNT > 0)) {
        if (inp.is_repeat(Action::MoveUp)) {
            cursor_--;
            if (cursor_ < 0) {
                cursor_ = lines - 1;
            }
            changed = true;
            SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveDown)) {
            cursor_++;
            if (cursor_ >= lines) {
                cursor_ = 0;
            }
            changed = true;
            SoundManager::instance().play_move();
        }
    }

    if (inp.is_triggered(Action::Decide) && BN_GENERATED_BGM_COUNT > 0) {
        test_bgm_id_ = static_cast<BgmId>(cursor_);
        screen_ = DebugScreen::BgmTest;
        last_drawn_bgm_playing_ = ! _bgm_test_track_is_playing();
        changed = true;
    }

    if (inp.is_triggered(Action::Cancel)) {
        screen_ = DebugScreen::Root;
        cursor_ = 1;
        changed = true;
    }

    if (changed) {
        redraw(ctx);
    }
}

void DebugState::draw_bgm_list(SharedContext& ctx) {
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "BGM Debug", sprites_);

    ctx.text_generator->set_left_alignment();
    int y = -48;
    const int spacing = 14;

    if (BN_GENERATED_BGM_COUNT == 0) {
        ctx.text_generator->generate(-112, y, "(no BGM)", sprites_);
    } else {
        for (unsigned i = 0; i < BN_GENERATED_BGM_COUNT; ++i) {
            bn::string<64> line;
            if (cursor_ == static_cast<int>(i)) {
                line.append(">");
            } else {
                line.append(" ");
            }
            line.append(bn::to_string<4>(static_cast<int>(i)));
            line.append(" ");
            line.append(audio_dispatch::bgm_name(static_cast<BgmId>(i)));
            ctx.text_generator->generate(-112, y, line, sprites_);
            y += spacing;
        }
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 76, "U/D A=test B=back", sprites_);
}

bool DebugState::_bgm_test_track_is_playing() const
{
    if (! bn::music::playing()) {
        return false;
    }
    bn::optional<bn::music_item> cur = bn::music::playing_item();
    if (! cur.has_value()) {
        return false;
    }
    return cur.value() == audio_dispatch::bgm_item(test_bgm_id_);
}

void DebugState::update_bgm_test(StateManager&, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    bool changed = false;

    if (inp.is_triggered(Action::Decide)) {
        if (_bgm_test_track_is_playing()) {
            SoundManager::instance().stop_bgm(0);
        } else {
            SoundManager::instance().play_bgm(test_bgm_id_, true, 0, true);
        }
        changed = true;
    }

    if (inp.is_triggered(Action::Cancel)) {
        SoundManager::instance().stop_bgm(0);
        screen_ = DebugScreen::BgmList;
        cursor_ = static_cast<int>(test_bgm_id_);
        if (cursor_ < 0 || cursor_ >= static_cast<int>(BN_GENERATED_BGM_COUNT)) {
            cursor_ = 0;
        }
        changed = true;
    }

    const bool playing = _bgm_test_track_is_playing();
    if (playing != last_drawn_bgm_playing_) {
        last_drawn_bgm_playing_ = playing;
        changed = true;
    }

    if (changed) {
        redraw(ctx);
    }
}

void DebugState::draw_bgm_test(SharedContext& ctx) {
    ctx.text_generator->set_center_alignment();
    bn::string<48> title;
    title.append("BGM ");
    title.append(audio_dispatch::bgm_name(test_bgm_id_));
    ctx.text_generator->generate(0, -76, title, sprites_);

    ctx.text_generator->set_left_alignment();
    bn::string<32> status;
    status.append(_bgm_test_track_is_playing() ? "PLAYING" : "STOPPED");
    ctx.text_generator->generate(-112, -40, status, sprites_);

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 56, "A=toggle play/stop", sprites_);
    ctx.text_generator->generate(0, 72, "B=stop+list", sprites_);
}

void DebugState::update_se_list(StateManager&, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    const int lines = (BN_GENERATED_SE_COUNT == 0) ? 1 : static_cast<int>(BN_GENERATED_SE_COUNT);
    bool changed = false;

    if (lines > 1 || (lines == 1 && BN_GENERATED_SE_COUNT > 0)) {
        if (inp.is_repeat(Action::MoveUp)) {
            cursor_--;
            if (cursor_ < 0) {
                cursor_ = lines - 1;
            }
            changed = true;
            SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveDown)) {
            cursor_++;
            if (cursor_ >= lines) {
                cursor_ = 0;
            }
            changed = true;
            SoundManager::instance().play_move();
        }
    }

    if (inp.is_triggered(Action::Decide) && BN_GENERATED_SE_COUNT > 0) {
        test_se_id_ = static_cast<SeId>(cursor_);
        screen_ = DebugScreen::SeTest;
        changed = true;
    }

    if (inp.is_triggered(Action::Cancel)) {
        screen_ = DebugScreen::Root;
        cursor_ = 2;
        changed = true;
    }

    if (changed) {
        redraw(ctx);
    }
}

void DebugState::draw_se_list(SharedContext& ctx) {
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "SE Debug", sprites_);

    ctx.text_generator->set_left_alignment();
    int y = -48;
    const int spacing = 14;

    if (BN_GENERATED_SE_COUNT == 0) {
        ctx.text_generator->generate(-112, y, "(no SE)", sprites_);
    } else {
        for (unsigned i = 0; i < BN_GENERATED_SE_COUNT; ++i) {
            bn::string<64> line;
            if (cursor_ == static_cast<int>(i)) {
                line.append(">");
            } else {
                line.append(" ");
            }
            line.append(bn::to_string<4>(static_cast<int>(i)));
            line.append(" ");
            line.append(audio_dispatch::se_name(static_cast<SeId>(i)));
            ctx.text_generator->generate(-112, y, line, sprites_);
            y += spacing;
        }
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 76, "U/D A=test B=back", sprites_);
}

void DebugState::update_se_test(StateManager&, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    bool changed = false;

    if (inp.is_triggered(Action::Decide)) {
        SoundManager::instance().play_se(test_se_id_);
        changed = true;
    }

    if (inp.is_triggered(Action::Cancel)) {
        screen_ = DebugScreen::SeList;
        cursor_ = static_cast<int>(test_se_id_);
        if (cursor_ < 0 || cursor_ >= static_cast<int>(BN_GENERATED_SE_COUNT)) {
            cursor_ = 0;
        }
        changed = true;
    }

    if (changed) {
        redraw(ctx);
    }
}

void DebugState::draw_se_test(SharedContext& ctx) {
    ctx.text_generator->set_center_alignment();
    bn::string<48> title;
    title.append("SE ");
    title.append(audio_dispatch::se_name(test_se_id_));
    ctx.text_generator->generate(0, -76, title, sprites_);

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -40, "A=play sample", sprites_);
    ctx.text_generator->generate(0, 72, "B=list", sprites_);
}

void DebugState::update_effect_test(StateManager&, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    bool changed = false;

    if (inp.is_triggered(Action::Decide)) {
        if (ctx.effect_manager) {
            // Aボタンでテストエフェクト(spr_dummy、回転・拡縮・跳ねるイージング)を画面中央にスポーン
            ctx.effect_manager->spawn(bn::sprite_items::spr_dummy, 0, 0, &test_preset);
        }
        changed = true;
    }

    if (inp.is_triggered(Action::Cancel)) {
        screen_ = DebugScreen::Root;
        cursor_ = 3;
        changed = true;
    }

    if (changed) {
        redraw(ctx);
    }
}

void DebugState::draw_effect_test(SharedContext& ctx) {
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "EFFECT DEBUG", sprites_);

    ctx.text_generator->generate(0, -40, "A = Spawn Test Effect", sprites_);
    ctx.text_generator->generate(0, -20, "(using Bounce / Rotate / Scale)", sprites_);
    ctx.text_generator->generate(0, 72, "B = back", sprites_);
}

// ==============================================================
// Event Debug (イベント再生テスト)
// ==============================================================

void DebugState::update_event_test(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    bool changed = false;

    if (inp.is_repeat(Action::MoveUp)) {
        event_cursor_--;
        if (event_cursor_ < 0) event_cursor_ = kEventCount - 1;
        changed = true;
        SoundManager::instance().play_move();
    }
    if (inp.is_repeat(Action::MoveDown)) {
        event_cursor_++;
        if (event_cursor_ >= kEventCount) event_cursor_ = 0;
        changed = true;
        SoundManager::instance().play_move();
    }

    if (inp.is_triggered(Action::Decide)) {
        // 選択したイベントを再生するために EventState へ遷移
        ctx.target_event_id = bn::string_view(g_events[event_cursor_].id);
        ctx.event_return_state = StateID::DEBUG_MENU;
        sm.change_state(StateID::EVENT);
        return;
    }

    if (inp.is_triggered(Action::Cancel)) {
        screen_ = DebugScreen::Root;
        cursor_ = 4;
        changed = true;
    }

    if (changed) {
        redraw(ctx);
    }
}

void DebugState::draw_event_test(SharedContext& ctx) {
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "EVENT DEBUG", sprites_);

    ctx.text_generator->set_left_alignment();
    const int spacing = 14;
    int y = -48;
    
    int start_i = event_cursor_ - 4;
    if (start_i < 0) start_i = 0;
    int end_i = start_i + 8;
    if (end_i > kEventCount) {
        end_i = kEventCount;
        start_i = end_i - 8;
        if (start_i < 0) start_i = 0;
    }

    for (int i = start_i; i < end_i; ++i) {
        bn::string<40> line;
        line.append(event_cursor_ == i ? ">" : " ");
        line.append(g_events[i].id);
        ctx.text_generator->generate(-112, y, line, sprites_);
        y += spacing;
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 72, "U/D A=play B=back", sprites_);
}

void DebugState::update_still_event_test(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    bool changed = false;

    if (inp.is_repeat(Action::MoveUp)) {
        event_cursor_--;
        if (event_cursor_ < 0) event_cursor_ = kStillEventCount - 1;
        changed = true;
        SoundManager::instance().play_move();
    }
    if (inp.is_repeat(Action::MoveDown)) {
        event_cursor_++;
        if (event_cursor_ >= kStillEventCount) event_cursor_ = 0;
        changed = true;
        SoundManager::instance().play_move();
    }

    if (inp.is_triggered(Action::Decide) && kStillEventCount > 0) {
        ctx.target_event_id = bn::string_view(g_still_events[event_cursor_].id);
        ctx.event_return_state = StateID::DEBUG_MENU;
        sm.change_state(StateID::STILL_EVENT);
        return;
    }

    if (inp.is_triggered(Action::Cancel)) {
        screen_ = DebugScreen::Root;
        cursor_ = 6;
        changed = true;
    }

    if (changed) {
        redraw(ctx);
    }
}

void DebugState::draw_still_event_test(SharedContext& ctx) {
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "STILL EVENT DEBUG", sprites_);

    ctx.text_generator->set_left_alignment();
    const int spacing = 14;
    int y = -48;
    
    if (kStillEventCount == 0) {
        ctx.text_generator->generate(-112, y, "(no still events)", sprites_);
    } else {
        int start_i = event_cursor_ - 4;
        if (start_i < 0) start_i = 0;
        int end_i = start_i + 8;
        if (end_i > kStillEventCount) {
            end_i = kStillEventCount;
            start_i = end_i - 8;
            if (start_i < 0) start_i = 0;
        }

        for (int i = start_i; i < end_i; ++i) {
            bn::string<64> line;
            line.append(event_cursor_ == i ? ">" : " ");
            line.append(g_still_events[i].id);
            ctx.text_generator->generate(-112, y, line, sprites_);
            y += spacing;
        }
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 72, "U/D A=play B=back", sprites_);
}

void DebugState::update_stage_list(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    bool changed = false;
    const int max_levels = get_num_levels();

    if (inp.is_repeat(Action::MoveUp)) {
        event_cursor_--;
        if (event_cursor_ < 0) event_cursor_ = max_levels - 1;
        changed = true;
        SoundManager::instance().play_move();
    }
    if (inp.is_repeat(Action::MoveDown)) {
        event_cursor_++;
        if (event_cursor_ >= max_levels) event_cursor_ = 0;
        changed = true;
        SoundManager::instance().play_move();
    }

    if (inp.is_triggered(Action::Decide)) {
        ctx.target_puzzle_level = event_cursor_;
        ctx.puzzle_return_state = StateID::DEBUG_MENU;
        sm.change_state(StateID::PUZZLE);
        return;
    }

    if (inp.is_triggered(Action::Cancel)) {
        screen_ = DebugScreen::Root;
        cursor_ = 5;
        changed = true;
    }

    if (changed) {
        redraw(ctx);
    }
}

void DebugState::draw_stage_list(SharedContext& ctx) {
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "STAGE DEBUG", sprites_);

    ctx.text_generator->set_left_alignment();
    const int spacing = 14;
    int y = -48;
    const int max_levels = get_num_levels();

    int start_i = event_cursor_ - 4;
    if (start_i < 0) start_i = 0;
    int end_i = start_i + 8;
    if (end_i > max_levels) {
        end_i = max_levels;
        start_i = end_i - 8;
        if (start_i < 0) start_i = 0;
    }

    for (int i = start_i; i < end_i; ++i) {
        bn::string<40> line;
        line.append(event_cursor_ == i ? ">" : " ");
        line.append("Level ");
        line.append(bn::to_string<4>(i));
        ctx.text_generator->generate(-112, y, line, sprites_);
        y += spacing;
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 72, "U/D A=play B=back", sprites_);
}

void DebugState::exit(StateManager& /*sm*/, SharedContext& ctx) {
    SoundManager& snd = SoundManager::instance();
    snd.stop_bgm(0);
    snd.set_bgm_enabled(prev_sound_bgm_enabled_);
    snd.set_se_enabled(prev_sound_se_enabled_);

    bn::backdrop::remove_color();
    FadeEffect::reset_palette();

    if (had_saved_sprite_intensity_) {
        bn::sprite_palettes::set_intensity(prev_sprite_intensity_);
        had_saved_sprite_intensity_ = false;
    }

    if (ctx.text_generator) {
        ctx.text_generator->set_palette_item(bn::sprite_items::japanese_font.palette_item());
    }

    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }

    portrait_solo_sprite_.reset();

    sprites_.clear();
}

// ============================================================
// AnimTest screen
// ============================================================
void DebugState::update_anim_test(StateManager& /*sm*/, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    const int count = static_cast<int>(SpriteAnimId::COUNT);
    bool changed = false;

    if (inp.is_repeat(Action::MoveUp)) {
        event_cursor_--;
        if (event_cursor_ < 0) event_cursor_ = count - 1;
        changed = true;
        SoundManager::instance().play_move();
        SpriteAnimManager::instance().stop(anim_test_handle_);
        anim_test_handle_ = INVALID_ANIM_HANDLE;
    }
    if (inp.is_repeat(Action::MoveDown)) {
        event_cursor_++;
        if (event_cursor_ >= count) event_cursor_ = 0;
        changed = true;
        SoundManager::instance().play_move();
        SpriteAnimManager::instance().stop(anim_test_handle_);
        anim_test_handle_ = INVALID_ANIM_HANDLE;
    }

    if (inp.is_triggered(Action::Decide)) {
        // 再生（無限ループ）
        SpriteAnimManager::instance().stop(anim_test_handle_);
        SpriteAnimId id = static_cast<SpriteAnimId>(event_cursor_);
        anim_test_handle_ = SpriteAnimManager::instance().play(id, 0, 0, -1);
        SpriteAnimManager::instance().set_bg_priority(anim_test_handle_, 0);
        changed = true;
    }
    if (inp.is_triggered(Action::Cancel)) {
        SpriteAnimManager::instance().stop(anim_test_handle_);
        anim_test_handle_ = INVALID_ANIM_HANDLE;
        screen_ = DebugScreen::Root;
        cursor_ = 5;
        changed = true;
    }

    if (changed) redraw(ctx);
}

void DebugState::draw_anim_test(SharedContext& ctx) {
    const int count = static_cast<int>(SpriteAnimId::COUNT);
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -76, "ANIM DEBUG", sprites_);

    ctx.text_generator->set_left_alignment();
    const int spacing = 13;
    int y = -50;

    int start_i = event_cursor_ - 4;
    if (start_i < 0) start_i = 0;
    int end_i = start_i + 8;
    if (end_i > count) { end_i = count; start_i = end_i - 8; if (start_i < 0) start_i = 0; }

    for (int i = start_i; i < end_i; ++i) {
        bn::string<64> line;
        line.append(event_cursor_ == i ? ">" : " ");
        const FdSpriteAnim& anim = g_sprite_anims[i];
        for (int c = 0; anim.id[c] != '\0' && c < 20; ++c) {
            line.push_back(anim.id[c]);
        }
        ctx.text_generator->generate(-112, y, line, sprites_);
        y += spacing;
    }

    ctx.text_generator->set_center_alignment();
    bn::string<32> playing_str;
    if (anim_test_handle_ != INVALID_ANIM_HANDLE &&
        SpriteAnimManager::instance().is_playing(anim_test_handle_)) {
        playing_str.append("PLAYING");
    } else {
        playing_str.append("A=play B=back");
    }
    ctx.text_generator->generate(0, 68, playing_str, sprites_);
}

// ============================================================
// ProgressData 画面（知識値デバッグ）
// ============================================================

// progress_sub_:
//   0 = トップメニュー（All ON / All OFF / イベント個別 / パズル個別 / ギャラリー解禁）
//   1 = イベント個別ON（g_events / g_still_events）
//   2 = パズル個別ON（レベルリスト）

void DebugState::update_progress_data(StateManager& /*sm*/, SharedContext& ctx) {
    if (!ctx.save) return;
    auto& inp = InputManager::instance();
    bool changed = false;

    SaveSlot& slot = ctx.save->slots[ctx.active_slot];

    if (progress_sub_ == 0) {
        // トップ: 5 items
        constexpr int items = 5;
        if (inp.is_repeat(Action::MoveUp)) {
            progress_cursor_--;
            if (progress_cursor_ < 0) progress_cursor_ = items - 1;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveDown)) {
            progress_cursor_++;
            if (progress_cursor_ >= items) progress_cursor_ = 0;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_triggered(Action::Decide)) {
            if (progress_cursor_ == 0) {
                // All Flags ON
                for (int i = 0; i < 32; ++i) slot.flags[i] = 0xFF;
                save_slot_save(*ctx.save, ctx.active_slot);
                changed = true;
            } else if (progress_cursor_ == 1) {
                // All Flags OFF
                for (int i = 0; i < 32; ++i) slot.flags[i] = 0x00;
                save_slot_save(*ctx.save, ctx.active_slot);
                changed = true;
            } else if (progress_cursor_ == 2) {
                // イベント個別
                progress_sub_ = 1;
                progress_cursor_ = 0;
                changed = true;
            } else if (progress_cursor_ == 3) {
                // パズル個別
                progress_sub_ = 2;
                progress_cursor_ = 0;
                changed = true;
            } else if (progress_cursor_ == 4 && ctx.save) {
                FixDataManager::instance().unlock_all_gallery_item_flags(
                    slot, *ctx.save, ctx.active_slot);
                changed = true;
            }
        }
    } else if (progress_sub_ == 1) {
        // イベント個別 (g_events + g_still_events)
        const int total = kEventCount + kStillEventCount;
        if (inp.is_repeat(Action::MoveUp)) {
            progress_cursor_--;
            if (progress_cursor_ < 0) progress_cursor_ = total - 1;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveDown)) {
            progress_cursor_++;
            if (progress_cursor_ >= total) progress_cursor_ = 0;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_triggered(Action::Decide) && total > 0) {
            // そのイベントIDの解禁ルールを適用（トグル）
            const char* eid = (progress_cursor_ < kEventCount)
                ? g_events[progress_cursor_].id
                : g_still_events[progress_cursor_ - kEventCount].id;
            FixDataManager::instance().toggle_event_unlocked(
                bn::string_view(eid), slot, *ctx.save, ctx.active_slot);
            changed = true;
        }
    } else if (progress_sub_ == 2) {
        // パズル個別
        const int max_levels = get_num_levels();
        if (inp.is_repeat(Action::MoveUp)) {
            progress_cursor_--;
            if (progress_cursor_ < 0) progress_cursor_ = max_levels - 1;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveDown)) {
            progress_cursor_++;
            if (progress_cursor_ >= max_levels) progress_cursor_ = 0;
            changed = true; SoundManager::instance().play_move();
        }
        if (inp.is_triggered(Action::Decide) && max_levels > 0) {
            UserDataPuzzle puzzle_data;
            user_data_puzzle_load(puzzle_data);
            
            bool current = puzzle_data.records[progress_cursor_].story_cleared;
            puzzle_data.records[progress_cursor_].story_cleared = !current;
            
            user_data_puzzle_save(puzzle_data);
            changed = true;
        }
    }

    if (inp.is_triggered(Action::Cancel)) {
        if (progress_sub_ == 0) {
            screen_ = DebugScreen::Root;
            cursor_ = 8;
            changed = true;
        } else {
            progress_sub_ = 0;
            progress_cursor_ = 0;
            changed = true;
        }
    }

    if (changed) redraw(ctx);
}

void DebugState::draw_progress_data(SharedContext& ctx) {
    if (!ctx.text_generator) return;
    ctx.text_generator->set_center_alignment();

    if (progress_sub_ == 0) {
        ctx.text_generator->generate(0, -76, "PROGRESS DATA", sprites_);
        ctx.text_generator->set_left_alignment();
        const char* items[] = {
            "All Flags ON",
            "All Flags OFF",
            "Event Flag (toggle)",
            "Puzzle Clear (toggle)",
            "Unlock gallery (save)",
        };
        int y = -32;
        for (int i = 0; i < 5; ++i) {
            bn::string<40> line;
            line.append(progress_cursor_ == i ? ">" : " ");
            line.append(items[i]);
            ctx.text_generator->generate(-112, y, line, sprites_);
            y += 14;
        }
        ctx.text_generator->set_center_alignment();
        ctx.text_generator->generate(0, 72, "U/D A=exec B=back", sprites_);

    } else if (progress_sub_ == 1) {
        ctx.text_generator->generate(0, -76, "EVENT FLAG", sprites_);
        ctx.text_generator->set_left_alignment();
        const int total = kEventCount + kStillEventCount;
        const int spacing = 14;
        int y = -48;
        int start_i = progress_cursor_ - 4;
        if (start_i < 0) start_i = 0;
        int end_i = start_i + 8;
        if (end_i > total) { end_i = total; start_i = end_i - 8; if (start_i < 0) start_i = 0; }
        for (int i = start_i; i < end_i; ++i) {
            bn::string<48> line;
            line.append(progress_cursor_ == i ? ">" : " ");
            const char* eid = (i < kEventCount)
                ? g_events[i].id
                : g_still_events[i - kEventCount].id;

            if (ctx.save && FixDataManager::instance().is_event_unlocked(bn::string_view(eid), ctx.save->slots[ctx.active_slot])) {
                line.append("[ON]  ");
            } else {
                line.append("[OFF] ");
            }

            line.append(eid);
            ctx.text_generator->generate(-112, y, line, sprites_);
            y += spacing;
        }
        ctx.text_generator->set_center_alignment();
        ctx.text_generator->generate(0, 72, "U/D A=toggle flag B=back", sprites_);
    } else {
        ctx.text_generator->generate(0, -76, "PUZZLE CLEAR", sprites_);
        ctx.text_generator->set_left_alignment();
        const int max_levels = get_num_levels();
        const int spacing = 14;
        int y = -48;
        int start_i = progress_cursor_ - 4;
        if (start_i < 0) start_i = 0;
        int end_i = start_i + 8;
        if (end_i > max_levels) { end_i = max_levels; start_i = end_i - 8; if (start_i < 0) start_i = 0; }
        UserDataPuzzle puzzle_data;
        user_data_puzzle_load(puzzle_data);

        for (int i = start_i; i < end_i; ++i) {
            bn::string<48> line;
            line.append(progress_cursor_ == i ? ">" : " ");

            if (puzzle_data.records[i].story_cleared) {
                line.append("[ON]  ");
            } else {
                line.append("[OFF] ");
            }

            line.append("Level ");
            line.append(bn::to_string<4>(i));
            ctx.text_generator->generate(-112, y, line, sprites_);
            y += spacing;
        }
        ctx.text_generator->set_center_alignment();
        ctx.text_generator->generate(0, 72, "U/D A=toggle clear B=back", sprites_);
    }
}

