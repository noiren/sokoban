#include "practice_menu_state.h"
#include "state/Manager/state_manager.h"
#include "input/input_manager.h"
#include "audio/sound_manager.h"
#include "game/levels.h"

#include "bn_backdrop.h"
#include "bn_color.h"
#include "bn_sprite_items_japanese_font.h"
#include "bn_string.h"

PracticeMenuState::PracticeMenuState()
    : cursor_(0), scroll_offset_(0) {
}

void PracticeMenuState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    // change_state で戻ってくる場合はカーソルをリセットしない
    // （puzzle_just_cleared が立っているなら前回のカーソル位置を保持したい）
    if (!ctx.puzzle_just_cleared) {
        cursor_ = 0;
        scroll_offset_ = 0;
    }

    // パズルユーザーデータをSRAMからロード
    user_data_puzzle_load(user_data_);

    // PuzzleState のクリア通知を受け取る
    if (ctx.puzzle_just_cleared) {
        ctx.puzzle_just_cleared = false;
        // 記録を更新してSRAMに保存
        user_data_puzzle_update(user_data_,
                                ctx.puzzle_clear_level,
                                ctx.puzzle_clear_moves,
                                ctx.puzzle_clear_frames);
        user_data_puzzle_save(user_data_);
    }

    bn::backdrop::set_color(bn::color(0, 0, 0));

    if (ctx.text_generator) {
        ctx.text_generator->set_palette_item(bn::sprite_items::japanese_font.palette_item());
    }

    sprites_.clear();
    redraw(ctx);
}

void PracticeMenuState::update(StateManager& sm, SharedContext& ctx) {
    auto& inp = InputManager::instance();
    const int max_levels = get_num_levels();

    // ストーリークリア済みステージリストを構築
    int cleared_levels[MAX_PUZZLE_RECORDS];
    int cleared_count = 0;
    for (int i = 0; i < max_levels && cleared_count < MAX_PUZZLE_RECORDS; ++i) {
        if (user_data_.records[i].story_cleared) {
            cleared_levels[cleared_count++] = i;
        }
    }

    bool changed = false;

    if (cleared_count > 0) {
        if (inp.is_repeat(Action::MoveUp)) {
            cursor_--;
            if (cursor_ < 0) cursor_ = cleared_count - 1;
            changed = true;
            SoundManager::instance().play_move();
        }
        if (inp.is_repeat(Action::MoveDown)) {
            cursor_++;
            if (cursor_ >= cleared_count) cursor_ = 0;
            changed = true;
            SoundManager::instance().play_move();
        }

        if (inp.is_triggered(Action::Decide)) {
            int actual_level = cleared_levels[cursor_];
            ctx.target_puzzle_level  = actual_level;
            ctx.puzzle_return_state  = StateID::PRACTICE;
            // 挑戦済みフラグを立ててSRAMに保存
            user_data_puzzle_set_attempted(user_data_, actual_level);
            user_data_puzzle_save(user_data_);
            sm.change_state(StateID::PUZZLE);
            return;
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

void PracticeMenuState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    sprites_.clear();
    bn::backdrop::remove_color();
}

void PracticeMenuState::resume(StateManager& /*sm*/, SharedContext& ctx) {
    // PuzzleState のクリア通知を受け取る
    if (ctx.puzzle_just_cleared) {
        ctx.puzzle_just_cleared = false;

        // 記録を更新してSRAMに保存
        user_data_puzzle_update(user_data_,
                                ctx.puzzle_clear_level,
                                ctx.puzzle_clear_moves,
                                ctx.puzzle_clear_frames);
        user_data_puzzle_save(user_data_);
    }

    // 画面を再描画
    sprites_.clear();
    redraw(ctx);
}

// ======================================================
// 描画
// ======================================================

void PracticeMenuState::redraw(SharedContext& ctx) {
    if (!ctx.text_generator) return;
    draw_stage_list(ctx);
}

void PracticeMenuState::draw_stage_list(SharedContext& ctx) {
    const int max_levels = get_num_levels();

    // ストーリークリア済みのステージだけを収集
    // （表示インデックス → 実レベル番号のマッピング）
    int cleared_levels[MAX_PUZZLE_RECORDS];
    int cleared_count = 0;
    for (int i = 0; i < max_levels && cleared_count < MAX_PUZZLE_RECORDS; ++i) {
        if (user_data_.records[i].story_cleared) {
            cleared_levels[cleared_count++] = i;
        }
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, -72, "STAGE DEBUG", sprites_);

    if (cleared_count == 0) {
        // まだストーリーでクリアしたステージがない
        ctx.text_generator->set_center_alignment();
        ctx.text_generator->generate(0, 0, "No stages cleared yet.", sprites_);
        ctx.text_generator->generate(0, 16, "Clear stages in Story mode!", sprites_);
        ctx.text_generator->set_center_alignment();
        ctx.text_generator->generate(0, 72, "B=menu", sprites_);
        return;
    }

    // カーソルをクリア済みリストの範囲内にクランプ
    if (cursor_ >= cleared_count) cursor_ = cleared_count - 1;

    // スクロール：カーソルが中央に来るよう調整
    constexpr int VISIBLE_LINES = 8;
    int start_i = cursor_ - (VISIBLE_LINES / 2);
    if (start_i < 0) start_i = 0;
    int end_i = start_i + VISIBLE_LINES;
    if (end_i > cleared_count) {
        end_i = cleared_count;
        start_i = end_i - VISIBLE_LINES;
        if (start_i < 0) start_i = 0;
    }

    ctx.text_generator->set_left_alignment();
    const int spacing = 13;
    int y = -44;

    for (int idx = start_i; idx < end_i; ++idx) {
        int level = cleared_levels[idx];
        bn::string<48> line;

        // カーソル
        line.append(cursor_ == idx ? ">" : " ");

        // ステージ番号
        line.append("Stage ");
        line.append(bn::to_string<4>(level));
        line.append(" ");

        // プラクティス記録表示（クリア時のみ）
        const PuzzleRecord& rec = user_data_.records[level];
        if (rec.attempted && rec.min_moves > 0) {
            line.append(bn::to_string<6>(rec.min_moves));
            line.append("te ");
            int secs = static_cast<int>(rec.best_frames) / 60;
            int frac = (static_cast<int>(rec.best_frames) % 60) * 10 / 60;
            line.append(bn::to_string<4>(secs));
            line.append(".");
            line.append(bn::to_string<2>(frac));
            line.append("s");
        }

        ctx.text_generator->generate(-112, y, line, sprites_);
        y += spacing;
    }

    ctx.text_generator->set_center_alignment();
    ctx.text_generator->generate(0, 72, "U/D A=play B=menu", sprites_);
}
