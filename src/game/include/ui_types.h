#pragma once

namespace ui_types {

    struct SpriteEntry {
        const char* id;
        const char* image_set;
        int image_no;
        float x;
        float y;
        float rotation;       // 角度 [0..360]
        bool visible;
    };

    // テキストの水平アライメント
    enum class TextAlign {
        LEFT   = 0,
        CENTER = 1,
        RIGHT  = 2,
    };

    struct TextEntry {
        const char* id;
        const char* text;
        float x, y;
        TextAlign align;
        float font_size;      // フォントスケール (1.0 = 標準, 1.1 = 1.1倍, ...)
        bool blink;
        int  blink_interval;
        bool visible;
    };

    // ==========================================
    // アニメーション用データ構造
    // ==========================================

    // アニメーションのグラフ（カーブ）の種類
    enum class EaseType {
        LINEAR = 0,     // 一定速度
        EASE_IN = 1,    // だんだん速く（加速）
        EASE_OUT = 2,   // だんだん遅く（減速・スライドイン向け）
        EASE_IN_OUT = 3,// S字カーブ
        CUBIC_IN = 4,
        CUBIC_OUT = 5,
        CUBIC_IN_OUT = 6,
        BACK_IN = 7,
        BACK_OUT = 8,
        BOUNCE_OUT = 9
    };

    // キーフレーム定義
    struct AnimKeyframe {
        int frame;
        float x;
        float y;
        float rot;
        float scale;
        EaseType ease_type; // 次のキーフレームへの補間方式
    };

    // アニメーションの「動き方」の定義（プリセット）
    struct AnimPreset {
        const char* id;           // プリセット名 (例: "slide_in_up")
        int duration_frames;      // かかるフレーム数
        EaseType ease_type;       // 全体の基本カーブ（下位互換/または基本用）
        int keyframe_count;       // キーフレーム数
        const AnimKeyframe* keyframes; // キーフレーム配列
    };

    // 画面に配置された「UIAnim」ノードのデータ
    struct AnimEntry {
        const char* id;             // このアニメーションノード自体のID (例: "main_anim")
        const char* preset_id;      // どのプリセットを使うか (例: "slide_in_up")
        
        int target_count;           // 動かす子ノードの数
        const char* const* target_ids; // 動かす子ノードのID配列へのポインタ
    };

    // ==========================================
    // 画面全体の構成データ
    // ==========================================

    struct ScreenData {
        const char* bg_image_id;
        int bg_scroll_x, bg_scroll_y;
        
        int sprite_count;
        const SpriteEntry* sprites;
        
        int text_count;
        const TextEntry* texts;

        // ★ アニメーション情報 ★
        int anim_preset_count;
        const AnimPreset* anim_presets; // 画面で使う動きの定義一覧

        int anim_entry_count;
        const AnimEntry* anim_entries;  // 実際に配置されたUIAnimノード一覧
    };

} // namespace ui_types