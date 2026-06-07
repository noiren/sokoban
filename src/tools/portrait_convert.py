#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
立ち絵変換の共有ロジック。

PNG(透過) → GBAスプライト用 64x64 / 8bitインデックス / 16色 / index0=マゼンタ透過 BMP。
GUI(portrait_maker.py) とプレビュー生成の両方から使う。

crop は元画像ピクセル単位の正方領域 {"left", "top", "side"}:
  - 画像範囲外を含んでもよい(範囲外は透過で埋まる)
  - これにより「パン(位置)＋ズーム(side)」での自由な切り出しを表現する
crop が None の場合は default_crop() でバストアップ相当を自動算出する。
"""
from PIL import Image

TARGET = 64
ALPHA_THRESH = 128


def default_crop(rgba):
    """アルファのbboxからバストアップ相当の正方crop(元px)を算出。"""
    bb = rgba.split()[3].getbbox()
    if not bb:
        w, h = rgba.size
        return {"left": 0, "top": 0, "side": min(w, h)}
    left, top, right, bottom = bb
    w = right - left
    # 幅を一辺とする正方形を上端から取る(顔～上半身が入る)
    side = w
    return {"left": int(left), "top": int(top), "side": int(side)}


def render_indexed(rgba, crop=None, target=TARGET, alpha_thresh=ALPHA_THRESH):
    """RGBA + crop → 'P'(8bit) 画像(index0=マゼンタ透過, 16色以内)。"""
    if crop is None:
        crop = default_crop(rgba)
    left = int(round(crop["left"]))
    top = int(round(crop["top"]))
    side = max(1, int(round(crop["side"])))

    # 範囲外は透過(0,0,0,0)で埋まる
    region = rgba.crop((left, top, left + side, top + side))
    region = region.resize((target, target), Image.LANCZOS)

    # アルファのハードマット: 可視はRGB保持、透過はマゼンタ(ピンク境界防止)
    r, g, b, a = region.split()
    hard_mask = a.point(lambda v: 255 if v >= alpha_thresh else 0)
    rgb = Image.composite(
        Image.merge("RGB", (r, g, b)),
        Image.new("RGB", region.size, (255, 0, 255)),
        hard_mask,
    )

    # 16色に量子化
    q = rgb.quantize(colors=16, method=Image.MEDIANCUT, dither=Image.NONE)
    pal = q.getpalette()[: 16 * 3]
    qd = list(q.getdata())

    # マゼンタに最も近いパレット番号を index0 とスワップ
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
    newpal[0:3] = [255, 0, 255]
    newpal += [0, 0, 0] * (256 - 16)

    # 透過ピクセルは強制 index0
    ad = list(a.getdata())
    qd = [0 if ad[i] < alpha_thresh else qd[i] for i in range(len(qd))]

    res = Image.new("P", (target, target))
    res.putpalette(newpal)
    res.putdata(qd)
    return res


def render_preview_rgba(rgba, crop=None, target=TARGET, alpha_thresh=ALPHA_THRESH):
    """変換後の見た目を RGBA で返す(GUIプレビュー用。透過は透明のまま)。"""
    p = render_indexed(rgba, crop, target, alpha_thresh)
    out = p.convert("RGBA")
    px = out.load()
    for y in range(out.height):
        for x in range(out.width):
            if px[x, y][:3] == (255, 0, 255):
                px[x, y] = (0, 0, 0, 0)
    return out


def convert_file(src_path, out_path, crop=None):
    """PNGファイル → BMPファイル へ変換して保存。"""
    rgba = Image.open(src_path).convert("RGBA")
    bmp = render_indexed(rgba, crop)
    bmp.save(out_path)
