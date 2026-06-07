#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
キャラ立ち絵(bustup)変換ツール。

透過PNGの全身立ち絵 → GBAスプライト用の 64x64 / 8bitインデックス / 16色 /
index0=マゼンタ透過 BMP へ変換する。

- バストアップ切り出し（顔～上半身）: 表情が読めるように上部を採用
- アルファのハードマット（半透明境界のピンクフリンジを防ぐ）
- 可視ピクセルを15色に量子化し、index0 をマゼンタ(透過)に固定

将来、元PNGを差し替えたら本スクリプトを再実行すればBMPが更新される。

実行: python src/tools/gen_chara_portraits.py
"""
import os
from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

SRC_DIR = os.path.join(PROJECT_ROOT, "Asset", "graphics", "png", "chara_bustup", "mayo")
DST_DIR = os.path.join(PROJECT_ROOT, "Asset", "graphics", "sprites", "chara", "chara")

TARGET = 64           # 出力サイズ(正方)
BUST_RATIO = 1.15     # bbox幅に対する切り出し高さ(上部から)。表情が読める範囲。
ALPHA_THRESH = 128    # これ未満を透過(index0)とみなす

# 元画像ファイル名の「マヨ_」以降のキー → 正準 image_id へのマッピング。
# 値が出力BMP/ image_id のベース(spr_ch_<value>.bmp, image_id=<value>)になる。
MAPPING = {
    # normal (ニュートラル)
    "Idle_1":        "mayo_normal_1",
    "Tickle_Idle_1": "mayo_normal_2",
    "Tickle_Idle_2": "mayo_normal_3",
    # smile (微笑・にやり)
    "Smirking_1":    "mayo_smile_1",
    "Smirking_2":    "mayo_smile_2",
    "Happy_2":       "mayo_smile_3",
    # sad
    "Sad_1":         "mayo_sad_1",
    "Sad_2":         "mayo_sad_2",
    "Sad_3":         "mayo_sad_3",
    # angry
    "Angry_1":       "mayo_angry_1",
    "Angry_2":       "mayo_angry_2",
    "Angry_3":       "mayo_angry_3",
    # surprised
    "Surprise_1":    "mayo_surprised_1",
    "Surprise_2":    "mayo_surprised_2",
    "Shame_1":       "mayo_surprised_3",
    # happy (喜び・大)
    "Happy_1":       "mayo_happy_1",
    "Happy_3":       "mayo_happy_2",
    "Happy_4":       "mayo_happy_3",
    # think (考え・興味)
    "Curious_1":     "mayo_think_1",
    "Curious_2":     "mayo_think_2",
    "Curious_3":     "mayo_think_3",
}


def find_source(suffix_key):
    """'Idle_1' のようなキーから、マヨ_<key>.png の実ファイルパスを探す。"""
    target = "_" + suffix_key + ".png"
    for f in os.listdir(SRC_DIR):
        if f.endswith(target):
            return os.path.join(SRC_DIR, f)
    return None


def convert(rgba):
    """RGBA立ち絵 → 64x64 8bitインデックスBMP(index0=マゼンタ透過, 16色)。"""
    # 1. アルファbboxで余白を除去
    bb = rgba.split()[3].getbbox()
    if bb:
        rgba = rgba.crop(bb)
    w, h = rgba.size

    # 2. バストアップ切り出し(上部)
    bust_h = min(h, int(w * BUST_RATIO))
    rgba = rgba.crop((0, 0, w, bust_h))
    w, h = rgba.size

    # 3. 64x64にアスペクト維持でフィット(上寄せ・中央揃え)
    s = min(TARGET / w, TARGET / h)
    nw, nh = max(1, round(w * s)), max(1, round(h * s))
    rgba = rgba.resize((nw, nh), Image.LANCZOS)
    canv = Image.new("RGBA", (TARGET, TARGET), (0, 0, 0, 0))
    canv.alpha_composite(rgba, ((TARGET - nw) // 2, 0))

    # 4. アルファのハードマット: 可視はRGB保持、透過はマゼンタ(ピンク境界防止)
    r, g, b, a = canv.split()
    hard_mask = a.point(lambda v: 255 if v >= ALPHA_THRESH else 0)
    rgb = Image.composite(
        Image.merge("RGB", (r, g, b)),
        Image.new("RGB", canv.size, (255, 0, 255)),
        hard_mask,
    )

    # 5. 16色に量子化
    q = rgb.quantize(colors=16, method=Image.MEDIANCUT, dither=Image.NONE)
    pal = q.getpalette()[: 16 * 3]
    qd = list(q.getdata())

    # 6. マゼンタに最も近いパレット番号を探して index0 とスワップ
    best, bestd = 0, 1 << 30
    for i in range(16):
        pr, pg, pb = pal[i * 3 : i * 3 + 3]
        d = (pr - 255) ** 2 + pg ** 2 + (pb - 255) ** 2
        if d < bestd:
            bestd, best = d, i
    swap = {best: 0, 0: best}
    qd = [swap.get(v, v) for v in qd]

    newpal = pal[:]
    newpal[0:3], newpal[best * 3 : best * 3 + 3] = (
        pal[best * 3 : best * 3 + 3],
        pal[0:3],
    )
    newpal[0:3] = [255, 0, 255]  # index0 を厳密にマゼンタに固定
    newpal += [0, 0, 0] * (256 - 16)

    # 7. 透過ピクセル(alpha<threshold)は強制的に index0
    ad = list(a.getdata())
    qd = [0 if ad[i] < ALPHA_THRESH else qd[i] for i in range(len(qd))]

    res = Image.new("P", (TARGET, TARGET))
    res.putpalette(newpal)
    res.putdata(qd)
    return res


def main():
    if not os.path.isdir(SRC_DIR):
        print(f"[gen_chara_portraits] Source not found: {SRC_DIR}")
        return
    os.makedirs(DST_DIR, exist_ok=True)

    done, missing = 0, []
    for key, out_base in MAPPING.items():
        src = find_source(key)
        if not src:
            missing.append(key)
            continue
        rgba = Image.open(src).convert("RGBA")
        bmp = convert(rgba)
        out_path = os.path.join(DST_DIR, f"spr_ch_{out_base}.bmp")
        bmp.save(out_path)
        done += 1
        print(f"  {key:14} -> spr_ch_{out_base}.bmp")

    print(f"[gen_chara_portraits] done: {done} written.")
    if missing:
        print(f"[gen_chara_portraits] WARNING missing sources: {missing}")


if __name__ == "__main__":
    main()
