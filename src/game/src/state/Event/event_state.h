#ifndef EVENT_STATE_H
#define EVENT_STATE_H

#include "state/state.h"
#include "save/save_data.h"
#include "bn_optional.h"
#include "bn_string_view.h"
#include "bn_regular_bg_ptr.h"
#include "ui/Core/Manager/ui_manager.h"
#include "generated/generated_fix_data.h"

// ========================================================
// EventUI - UIManagerへの高レベルアクセサ
// ========================================================
class EventUI {
public:
    EventUI(UIManager& ui) : ui_(ui) {}

    void set_name(const bn::string_view& name) {
        if (auto* text_node = ui_.get_text("name_box")) {
            text_node->set_text(name);
        }
    }

    void set_message(const bn::string_view& msg) {
        if (auto* text_node = ui_.get_text("message_text")) {
            text_node->set_text(msg);
        }
    }

    // 左キャラクター表示 (そのまま・右向き画像を使用)
    void set_left_char(const bn::string_view& image_id) {
        if (auto* img_node = ui_.get_image("char_left")) {
            ui_.change_sprite_image_by_id(img_node, image_id);
            img_node->set_visible(true);
            img_node->set_horizontal_flip(false);
        }
    }

    // 右キャラクター表示 (水平反転して中央向きにする)
    void set_right_char(const bn::string_view& image_id) {
        if (auto* img_node = ui_.get_image("char_right")) {
            ui_.change_sprite_image_by_id(img_node, image_id);
            img_node->set_visible(true);
            img_node->set_horizontal_flip(true); // 反転して中央を向かせる
        }
    }

    void clear_left_char() {
        if (auto* img_node = ui_.get_image("char_left")) {
            img_node->set_visible(false);
        }
    }

    void clear_right_char() {
        if (auto* img_node = ui_.get_image("char_right")) {
            img_node->set_visible(false);
        }
    }

    void clear_chars() {
        clear_left_char();
        clear_right_char();
    }

    // メッセージ送りアイコンの表示/非表示
    void set_next_icon_visible(bool visible) {
        if (auto* img_node = ui_.get_image("next_icon")) {
            img_node->set_visible(visible);
        }
    }

private:
    UIManager& ui_;
};

// ========================================================
// EventPhase
// ========================================================
enum class EventPhase {
    TYPING,        // テキストをタイプライター表示中
    WAITING_INPUT, // 全文表示完了→入力待ち
    FINISHED,      // イベント終了処理
    COUNT
};

// ========================================================
// EventState
// ========================================================
class EventState : public State {
public:
    // タイピング速度定数 (フレーム/文字)
    static constexpr int TYPING_SPEED_NORMAL = 2; // 通常: 3フレームに1文字
    static constexpr int TYPING_SPEED_FAST   = 1; // A長押し: 1フレームに1文字 (ビュ～)
    static constexpr int TYPING_SPEED_INSTANT = 0; // Bボタン: 即時全表示

    EventState();

    void enter(StateManager& sm, SharedContext& ctx) override;
    void update(StateManager& sm, SharedContext& ctx) override;
    void exit(StateManager& sm, SharedContext& ctx) override;
    void pause(StateManager& sm, SharedContext& ctx) override {}
    void resume(StateManager& sm, SharedContext& ctx) override {}

private:
    void change_phase(EventPhase next);

    void enter_typing();
    void update_typing(StateManager& sm, SharedContext& ctx);
    void exit_typing();

    void enter_waiting();
    void update_waiting(StateManager& sm, SharedContext& ctx);
    void exit_waiting();

    void enter_finished();
    void update_finished(StateManager& sm, SharedContext& ctx);
    void exit_finished();

    // 1行分のイベントラインを適用する (キャラ表示, 名前表示, テキスト開始)
    void apply_line(int line_index);

    // タイプライター表示更新 (1フレーム分を進める)
    void advance_typewriter(int chars_per_frame);

    // 現在のtypewrite_text_ を displayed_chars_ 文字分だけ UIに適用
    void refresh_display_text();

    using EnterExitFunc = void (EventState::*)();
    using UpdateFunc = void (EventState::*)(StateManager&, SharedContext&);

    struct PhaseHandlers {
        EnterExitFunc enter;
        UpdateFunc    update;
        EnterExitFunc exit;
    };

    static const PhaseHandlers phase_table_[];

    // FixDataManager から取得したイベント定義ポインタ (EWRAMに存在する定数テーブルを指す)
    const FdEventEntry* event_entry_;
    int pc_;           // 現在再生中の行インデックス
    EventPhase phase_;

    // タイプライター制御
    bn::string_view typewrite_text_; // 現在表示中のフルテキスト (FdEventLineのtextを指すビュー)
    int displayed_chars_;            // 現在何文字まで表示したか
    int text_timer_;                 // 次の文字を出すまでのカウントダウン
    bool skip_requested_;            // Bボタン単押しによる瞬時表示フラグ
    bool force_skip_;                // Bボタン長押しによる全スキップフラグ

    // UI管理
    bn::optional<UIManager> ui_manager_;
    bn::optional<EventUI> ui_;
    PhaseStep step_;
    bn::optional<bn::regular_bg_ptr> msg_window_bg_;
};

#endif // EVENT_STATE_H
