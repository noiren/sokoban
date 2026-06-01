#include "event_state.h"

#include "state/Manager/state_manager.h"
#include "state/state_id.h"
#include "input/input_manager.h"
#include "fixdata/fix_data_manager.h"
#include "ui_data_event.h"

#include "bn_keypad.h"
#include "bn_regular_bg_items_ui_msg_window.h"

namespace {

int get_utf8_char_count(bn::string_view str) {
    int count = 0;
    int len = str.size();
    for (int i = 0; i < len; ++i) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if ((c & 0xC0) != 0x80) {
            count++;
        }
    }
    return count;
}

int get_utf8_byte_index(bn::string_view str, int char_limit) {
    int byte_idx = 0;
    int char_count = 0;
    int len = str.size();
    while (byte_idx < len && char_count < char_limit) {
        unsigned char c = static_cast<unsigned char>(str[byte_idx]);
        if ((c & 0xC0) != 0x80) {
            char_count++;
            if (char_count > char_limit) {
                break;
            }
        }
        byte_idx++;
    }
    while (byte_idx < len) {
        unsigned char c = static_cast<unsigned char>(str[byte_idx]);
        if ((c & 0xC0) != 0x80) {
            break;
        }
        byte_idx++;
    }
    return byte_idx;
}

} // namespace

// ============================================================

// フェーズテーブル
// ============================================================
const EventState::PhaseHandlers EventState::phase_table_[] = {
    // TYPING
    { &EventState::enter_typing, &EventState::update_typing, &EventState::exit_typing },
    // WAITING_INPUT
    { &EventState::enter_waiting, &EventState::update_waiting, &EventState::exit_waiting },
    // FINISHED
    { &EventState::enter_finished, &EventState::update_finished, &EventState::exit_finished }
};

// ============================================================
// コンストラクタ
// ============================================================
EventState::EventState()
    : event_entry_(nullptr), pc_(0), phase_(EventPhase::TYPING),
      typewrite_text_(), displayed_chars_(0), text_timer_(0),
      skip_requested_(false), force_skip_(false),
      step_(PhaseStep::OPENING) {
}

// ============================================================
// enter / update / exit (State ライフサイクル)
// ============================================================
void EventState::enter(StateManager& /*sm*/, SharedContext& ctx) {
    // UI 初期化
    ui_manager_.emplace(*ctx.text_generator);
    ui_manager_->load_screen(ui_data_event::SCREEN);
    ui_.emplace(*ui_manager_);

    // メッセージウィンドウ背景のロードと優先度(Priority 1)設定
    msg_window_bg_ = bn::regular_bg_items::ui_msg_window.create_bg(8, 48);
    msg_window_bg_->set_priority(1);

    // 各UIノードの優先度を個別に制御して、正しくレイヤー化
    if (auto* text_node = ui_manager_->get_text("name_box")) {
        text_node->set_bg_priority(0);
    }
    if (auto* text_node = ui_manager_->get_text("message_text")) {
        text_node->set_bg_priority(0);
    }
    if (auto* img_node = ui_manager_->get_image("next_icon")) {
        img_node->set_bg_priority(0);
    }
    if (auto* img_node = ui_manager_->get_image("char_left")) {
        img_node->set_bg_priority(2);
    }
    if (auto* img_node = ui_manager_->get_image("char_right")) {
        img_node->set_bg_priority(2);
    }

    // イベントデータを FixDataManager から取得
    event_entry_ = FixDataManager::instance().find_event(ctx.target_event_id);

    pc_ = 0;
    skip_requested_ = false;
    force_skip_ = false;
    phase_ = EventPhase::TYPING;
    step_ = PhaseStep::OPENING;

    // フェーズ開始
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

void EventState::update(StateManager& sm, SharedContext& ctx) {
    if (phase_table_[(int)phase_].update) {
        (this->*phase_table_[(int)phase_].update)(sm, ctx);
    }

    if (ui_manager_) {
        ui_manager_->update();
    }
}

void EventState::exit(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }

    ui_.reset();
    msg_window_bg_.reset();
    if (ui_manager_) {
        ui_manager_->clear_all();
        ui_manager_.reset();
    }
}

// ============================================================
// フェーズ遷移
// ============================================================
void EventState::change_phase(EventPhase next) {
    if (phase_table_[(int)phase_].exit) {
        (this->*phase_table_[(int)phase_].exit)();
    }
    phase_ = next;
    if (phase_table_[(int)phase_].enter) {
        (this->*phase_table_[(int)phase_].enter)();
    }
}

// ============================================================
// TYPING フェーズ
// ============================================================
void EventState::enter_typing() {
    // 現在の行を UI に適用する
    apply_line(pc_);

    skip_requested_ = false;
    // 文字タイマーを 0 にしてすぐ最初の文字を表示
    text_timer_ = 0;
    step_ = PhaseStep::RUNNING;
}

void EventState::update_typing(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    // ---- 入力チェック ----
    // B長押し: 全スキップモードに入る
    if (bn::keypad::b_held()) {
        force_skip_ = true;
    }

    // B単押し: 現在のテキストを瞬時全表示
    if (bn::keypad::b_pressed()) {
        skip_requested_ = true;
    }

    // A長押し: 高速表示 (ビュ〜)
    const bool a_held = bn::keypad::a_held();

    // ---- テキスト進行 ----
    if (skip_requested_ || force_skip_) {
        // 一気に全文字表示して入力待ちへ
        displayed_chars_ = get_utf8_char_count(typewrite_text_);
        refresh_display_text();
        skip_requested_ = false;
        change_phase(EventPhase::WAITING_INPUT);
        return;
    }

    // 全文字表示済みなら自動で入力待ちへ
    if (displayed_chars_ >= get_utf8_char_count(typewrite_text_)) {
        change_phase(EventPhase::WAITING_INPUT);
        return;
    }

    // タイマーカウントダウン
    text_timer_--;
    if (text_timer_ <= 0) {
        // A長押し: 1フレームに1文字(ビュ〜)
        // 通常: TYPING_SPEED_NORMAL フレームに1文字
        const int speed = a_held ? TYPING_SPEED_FAST : TYPING_SPEED_NORMAL;
        text_timer_ = speed;
        advance_typewriter(1);
    }
}

void EventState::exit_typing() {}

// ============================================================
// WAITING_INPUT フェーズ
// ============================================================
void EventState::enter_waiting() {
    // 確実に全文表示
    displayed_chars_ = get_utf8_char_count(typewrite_text_);
    refresh_display_text();

    // メッセージ送りアイコンを表示
    if (ui_) {
        ui_->set_next_icon_visible(true);
    }
    step_ = PhaseStep::RUNNING;
}

void EventState::update_waiting(StateManager& /*sm*/, SharedContext& /*ctx*/) {
    // B長押し: 全スキップモードのまま自動で次へ
    if (bn::keypad::b_held()) {
        force_skip_ = true;
    }

    // A単押し または B長押し状態で次の行へ
    const bool decide = InputManager::instance().is_triggered(Action::Decide);
    const bool b_pressed = bn::keypad::b_pressed();

    if (decide || force_skip_ || b_pressed) {
        // メッセージ送りアイコンを非表示に
        if (ui_) {
            ui_->set_next_icon_visible(false);
        }

        // 次の行へ
        pc_++;
        if (!event_entry_ || pc_ >= event_entry_->line_count) {
            // イベント終了
            change_phase(EventPhase::FINISHED);
        } else {
            // 次の行の TYPING へ
            change_phase(EventPhase::TYPING);
        }
    }
}

void EventState::exit_waiting() {
    if (ui_) {
        ui_->set_next_icon_visible(false);
    }
}

// ============================================================
// FINISHED フェーズ
// ============================================================
void EventState::enter_finished() {
    // キャラクターを隠す
    if (ui_) {
        ui_->clear_chars();
    }
    step_ = PhaseStep::RUNNING;
}

void EventState::update_finished(StateManager& sm, SharedContext& ctx) {
    // ストーリーモードから呼ばれた場合は pop_state() で StoryState の resume() を起動
    // それ以外（メニューから直接など）は従来通り change_state
    if (ctx.event_return_state == StateID::STORY) {
        ctx.story_step_completed = true;
        sm.pop_state();
    } else {
        sm.change_state(ctx.event_return_state);
    }
}

void EventState::exit_finished() {}

// ============================================================
// 内部ヘルパー
// ============================================================

// 1行分のイベントデータを UI に適用する
void EventState::apply_line(int line_index) {
    if (!event_entry_ || !ui_) return;
    if (line_index < 0 || line_index >= event_entry_->line_count) return;

    const FdEventLine& line = event_entry_->lines[line_index];

    // キャラクター名の表示 (FixDataManager から取得)
    const FdCharacterEntry* chara = FixDataManager::instance().find_character(
        bn::string_view(line.speaker_id));
    if (chara) {
        ui_->set_name(bn::string_view(chara->name_ja));
    } else {
        ui_->set_name(bn::string_view());
    }

    // 立ち絵の表示 (position に応じて左右に配置)
    bn::string_view image_id(line.image_id);
    if (line.position == FdPosition::Left) {
        ui_->set_left_char(image_id);
    } else {
        ui_->set_right_char(image_id);
    }

    // テキスト設定 (タイプライター用: 最初は空)
    typewrite_text_ = bn::string_view(line.text);
    displayed_chars_ = 0;
    text_timer_ = TYPING_SPEED_NORMAL;
    refresh_display_text();
}

// タイプライター: chars_per_frame 文字分だけ進める
void EventState::advance_typewriter(int chars_per_frame) {
    displayed_chars_ += chars_per_frame;
    int max_chars = get_utf8_char_count(typewrite_text_);
    if (displayed_chars_ > max_chars) {
        displayed_chars_ = max_chars;
    }
    refresh_display_text();
}

// UIのメッセージテキストを displayed_chars_ 文字分だけ表示更新する
void EventState::refresh_display_text() {
    if (!ui_) return;

    // 表示文字数 (UTF-8文字数) に対応するバイト長を取得してスライス
    int bytes = get_utf8_byte_index(typewrite_text_, displayed_chars_);
    bn::string_view partial(typewrite_text_.data(), (unsigned)bytes);
    ui_->set_message(partial);
}
