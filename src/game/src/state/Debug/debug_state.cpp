#include "debug_state.h"

#include "state/Manager/state_manager.h"
#include "input/input_manager.h"
#include "audio/sound_manager.h"
#include "generated/audio_dispatch_bgm.gen.h"
#include "ui/Core/Effects/fade_effect.h"

#include "bn_backdrop.h"
#include "bn_color.h"
#include "bn_music.h"
#include "bn_music_item.h"
#include "bn_sprite_items_japanese_font.h"
#include "bn_sprite_palettes.h"
#include "bn_string.h"

namespace {

[[nodiscard]] const char* bgm_line_label(int index)
{
    if (index < 0 || index >= static_cast<int>(BN_GENERATED_BGM_COUNT)) {
        return "?";
    }
    const auto id = static_cast<BgmId>(index);
    switch (id) {
        case BgmId::Afterburner:
            return "Afterburner";
        case BgmId::RollinDownTheStreet:
            return "Rollin";
        case BgmId::FlowerGuysPoolParty:
            return "FlowerGuy";
        case BgmId::COUNT:
        default:
            return "?";
    }
}

[[nodiscard]] const char* se_line_label(int index)
{
    if (index < 0 || index >= static_cast<int>(BN_GENERATED_SE_COUNT)) {
        return "?";
    }
    const auto id = static_cast<SeId>(index);
    switch (id) {
        case SeId::Default_Move:
            return "Move";
        case SeId::Default_Push:
            return "Push";
        case SeId::Default_Clear:
            return "Clear";
        case SeId::Default_Reset:
            return "Reset";
        case SeId::COUNT:
        default:
            return "?";
    }
}

} // namespace

DebugState::DebugState()
    : screen_(DebugScreen::Root),
      cursor_(0),
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
    last_drawn_bgm_playing_ = false;

    SoundManager& snd = SoundManager::instance();
    prev_sound_bgm_enabled_ = snd.bgm_enabled();
    prev_sound_se_enabled_ = snd.se_enabled();
    snd.set_bgm_enabled(true);
    snd.set_se_enabled(true);

    bn::backdrop::set_color(bn::color(0, 0, 0));
    FadeEffect::reset_palette();

    prev_sprite_intensity_ = bn::sprite_palettes::intensity();
    had_saved_sprite_intensity_ = true;
    bn::sprite_palettes::set_intensity(bn::fixed(1));

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
        default:
            break;
    }
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
        default:
            break;
    }
}

void DebugState::update_root(StateManager& sm, SharedContext& ctx) {
    constexpr int lines = 3;
    auto& inp = InputManager::instance();
    bool changed = false;

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

    if (inp.is_triggered(Action::Decide)) {
        if (cursor_ == 0) {
            // UI Debug: no-op
        } else if (cursor_ == 1) {
            screen_ = DebugScreen::BgmList;
            cursor_ = 0;
            changed = true;
        } else if (cursor_ == 2) {
            screen_ = DebugScreen::SeList;
            cursor_ = 0;
            changed = true;
        }
    }

    if (inp.is_triggered(Action::Cancel)) {
        sm.change_state(StateID::MENU);
        return;
    }

    if (changed) {
        redraw(ctx);
    }
}

void DebugState::draw_root(SharedContext& ctx) {
    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -72, "DEBUG", sprites_);

    ctx.text_generator->set_left_alignment();
    const int spacing = 14;
    int y = -40;
    for (int i = 0; i < 3; ++i) {
        bn::string<32> line;
        if (cursor_ == i) {
            line.append(">");
        } else {
            line.append(" ");
        }
        switch (i) {
            case 0:
                line.append("UI Debug");
                break;
            case 1:
                line.append("BGM Debug");
                break;
            default:
                line.append("SE Debug");
                break;
        }
        ctx.text_generator->generate(-112, y, line, sprites_);
        y += spacing;
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 72, "U/D A=open B=menu", sprites_);
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
            bn::string<36> line;
            if (cursor_ == static_cast<int>(i)) {
                line.append(">");
            } else {
                line.append(" ");
            }
            line.append(bn::to_string<4>(static_cast<int>(i)));
            line.append(" ");
            line.append(bgm_line_label(static_cast<int>(i)));
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
    title.append(bgm_line_label(static_cast<int>(test_bgm_id_)));
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
            bn::string<36> line;
            if (cursor_ == static_cast<int>(i)) {
                line.append(">");
            } else {
                line.append(" ");
            }
            line.append(bn::to_string<4>(static_cast<int>(i)));
            line.append(" ");
            line.append(se_line_label(static_cast<int>(i)));
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
    title.append(se_line_label(static_cast<int>(test_se_id_)));
    ctx.text_generator->generate(0, -76, title, sprites_);

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -40, "A=play sample", sprites_);
    ctx.text_generator->generate(0, 72, "B=list", sprites_);
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

    sprites_.clear();
}
