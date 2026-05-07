#ifndef FADE_EFFECT_H
#define FADE_EFFECT_H

#include "bn_color.h"
#include "bn_fixed.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_palettes.h"

// フェードイン/アウトを簡潔に扱うユーティリティ
// 使い方:
//   FadeEffect fade_;
//   // 毎フレーム:
//   if (fade_.is_active()) { fade_.update(); }
//   // 開始:
//   fade_.start_fade_in(30);  // or fade_out
class FadeEffect {
public:
    FadeEffect() = default;

    /// フェードイン開始（真っ黒→通常表示）
    /// \param duration フレーム数
    void start_fade_in(int duration) {
        fading_in_ = true;
        fading_out_ = false;
        frame_ = 0;
        duration_ = duration;
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), 1);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 1);
        active_ = true;
    }

    /// フェードアウト開始（通常表示→真っ黒）
    /// \param duration フレーム数
    void start_fade_out(int duration) {
        fading_in_ = false;
        fading_out_ = true;
        frame_ = 0;
        duration_ = duration;
        active_ = true;
    }

    /// 毎フレーム呼び出す。trueを返す間はフェード中、falseで完了。
    bool update() {
        if (!active_) return false;

        frame_++;
        bn::fixed fade;

        if (fading_in_) {
            // 1 → 0 へ
            fade = bn::fixed(1) - bn::fixed(frame_) / duration_;
            if (fade <= 0) {
                bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
                bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 0);
                active_ = false;
                return false;
            }
        } else {
            // 0 → 1 へ
            fade = bn::fixed(frame_) / duration_;
            if (fade >= 1) {
                bn::bg_palettes::set_fade(bn::color(0, 0, 0), 1);
                bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 1);
                active_ = false;
                return false;
            }
        }

        bn::bg_palettes::set_fade(bn::color(0, 0, 0), fade);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), fade);
        return true;  // フェード継続中
    }

    /// フェードが進行中か
    bool is_active() const { return active_; }
    /// フェードイン中か
    bool is_fading_in() const { return active_ && fading_in_; }
    /// フェードアウト中か
    bool is_fading_out() const { return active_ && fading_out_; }

    /// 即座にパレットを通常状態に戻す（shutdown時など）
    static void reset_palette() {
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 0);
    }

private:
    bool active_ = false;
    bool fading_in_ = false;
    bool fading_out_ = false;
    int frame_ = 0;
    int duration_ = 30;
};

#endif // FADE_EFFECT_H
