#include "effect_manager.h"
#include "ui_anim_math.h"
#include "bn_sprite_item.h"
#include "bn_sprite_double_size_mode.h"

EffectManager::EffectManager() {
    effects_.resize(MAX_EFFECTS); //起動時に固定長確保
}

void EffectManager::spawn(const bn::sprite_item& sprite_item, 
                          bn::fixed x, bn::fixed y, 
                          const ui_types::AnimPreset* preset) 
{
    for (auto& eff : effects_) {
        if (!eff.active) {
            eff.active = true;
            eff.timer = 0;
            eff.base_x = x;
            eff.base_y = y;
            eff.preset = preset;
            
            // スプライト生成
            eff.sprite = sprite_item.create_sprite(x, y);

            // インテリジェント安全装置：プリセットに回転/拡縮が含まれるかチェック
            eff.use_affine = AnimMath::requires_affine(preset);
            
            if (eff.use_affine && eff.sprite) {
                // 回転・拡大時に画像がスプライト枠外でクリップ（切れる）のを防ぐため、
                // Butanoのダブルサイズモード(ハードウェア機能)を有効化する
                eff.sprite->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            }

            // タイルのコマ数が1より大きい場合のみ、パラパラアニメーションのアクションを生成
            int graphics_count = sprite_item.tiles_item().graphics_count();
            if (eff.sprite && graphics_count > 1) {
                // graphics_animate_action<4> は4つのインデックスを必要とするため、コマ数に応じてインデックスを安全に補完
                int i0 = 0;
                int i1 = (graphics_count > 1) ? 1 : 0;
                int i2 = (graphics_count > 2) ? 2 : i1;
                int i3 = (graphics_count > 3) ? 3 : i2;

                eff.frame_anim = bn::create_sprite_animate_action_once(
                    *eff.sprite, 6, sprite_item.tiles_item(), i0, i1, i2, i3
                );
            } else {
                eff.frame_anim.reset();
            }
            break;
        }
    }
}

void EffectManager::update() {
    for (auto& eff : effects_) {
        if (!eff.active) continue;

        eff.timer++;
        bool is_finished = false;

        // 1. パラパラ漫画アニメーションの更新
        if (eff.frame_anim) {
            eff.frame_anim->update();
            // アニメの1周が終了し、かつTRSのPresetが設定されていない場合は寿命とする
            if (eff.frame_anim->done() && !eff.preset) {
                is_finished = true;
            }
        }

        // 2. エディタで作ったTRS(移動・回転・拡縮)の更新
        if (eff.preset && eff.preset->keyframe_count > 0 && eff.sprite) {
            AnimMath::Transform t = AnimMath::evaluate(eff.preset, eff.timer);
            
            eff.sprite->set_position(eff.base_x + t.x, eff.base_y + t.y);
            
            // アフィンが必要な時(use_affine)のみ、回転とスケールをセットする。
            // ※これを徹底することで、移動だけのエフェクトはアフィン行列を消費せず、GBAがクラッシュしません。
            if (eff.use_affine) {
                eff.sprite->set_rotation_angle(t.rot);
                eff.sprite->set_scale(t.scale);
            }

            // 指定フレーム経過で終了
            if (eff.timer >= eff.preset->duration_frames) {
                is_finished = true;
            }
        } else if (eff.timer > 120) {
            // パラパラアニメもPresetも設定されていない場合の安全措置（約2秒で消す）
            is_finished = true;
        }

        // 3. 終了・破棄処理
        if (is_finished) {
            eff.active = false;
            eff.frame_anim.reset();
            eff.sprite.reset(); // VRAM/OAM/アフィン行列 の解放
        }
    }
}
