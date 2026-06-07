#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GBA 向け画像焼き込み（スプライト / スチル背景）共有ロジック。

- スプライト: 8bit インデックス BMP、最大16色、index0=マゼンタ透過、GBA規定サイズのみ
- スチル: 8bit インデックス BMP、最大256色（イベント奥レイヤー併用時は240色警告）、
          index0 は絵に使わない（予約色）

UI エディタ検証・汎用焼きタブ・CLI から利用する。
"""
from __future__ import annotations

import os
from collections import Counter
from typing import Literal

from PIL import Image

# ASSET_GUIDELINES.md 準拠（正方形・横長・縦長）
ALLOWED_SPRITE_SIZES: frozenset[tuple[int, int]] = frozenset(
    {
        (8, 8),
        (16, 16),
        (32, 32),
        (64, 64),
        (16, 8),
        (32, 8),
        (64, 8),
        (8, 16),
        (8, 32),
        (8, 64),
        (32, 16),
        (32, 64),
        (16, 32),
        (64, 32),
    }
)

STILL_DEFAULT_SIZE = (256, 256)
STILL_EVENT_BG_MAX_COLORS = 240  # ガイドライン: 奥レイヤー + UI16色


def _letterbox_rgba(rgba: Image.Image, tw: int, th: int) -> Image.Image:
    """アスペクト維持で tw×th に収め、余白は透明。"""
    w, h = rgba.size
    s = min(tw / max(1, w), th / max(1, h))
    nw, nh = max(1, int(round(w * s))), max(1, int(round(h * s)))
    sm = rgba.resize((nw, nh), Image.Resampling.LANCZOS)
    out = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
    ox, oy = (tw - nw) // 2, (th - nh) // 2
    out.alpha_composite(sm, (ox, oy))
    return out


def _stretch_rgba(rgba: Image.Image, tw: int, th: int) -> Image.Image:
    return rgba.resize((tw, th), Image.Resampling.LANCZOS)


def bake_sprite(
    rgba: Image.Image,
    tw: int,
    th: int,
    *,
    alpha_thresh: int = 128,
    fit: Literal["letterbox", "stretch"] = "letterbox",
) -> tuple[Image.Image, list[str]]:
    """
    RGBA → 8bit P、16色、index0=マゼンタ透過。
    戻り: (BMP用 Image 'P', 警告メッセージ一覧)
    """
    warnings: list[str] = []
    if (tw, th) not in ALLOWED_SPRITE_SIZES:
        warnings.append(
            f"サイズ {tw}×{th} は GBA スプライトの規定外です（8〜64 の組み合わせ一覧に従ってください）。"
        )

    if fit == "stretch":
        work = _stretch_rgba(rgba, tw, th)
    else:
        work = _letterbox_rgba(rgba, tw, th)

    r, g, b, a = work.split()
    hard_mask = a.point(lambda v: 255 if v >= alpha_thresh else 0)
    rgb = Image.composite(
        Image.merge("RGB", (r, g, b)),
        Image.new("RGB", work.size, (255, 0, 255)),
        hard_mask,
    )

    q = rgb.quantize(colors=16, method=Image.MEDIANCUT, dither=Image.NONE)
    pal = q.getpalette()[: 48]
    qd = list(q.getdata())

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

    ad = list(a.getdata())
    qd = [0 if ad[i] < alpha_thresh else qd[i] for i in range(len(qd))]

    used = len({x for x in qd if x != 0})
    if used > 15:
        warnings.append(f"実質使用色が多い可能性: 透過除き {used} 色（推奨15色以下）。")

    res = Image.new("P", (tw, th))
    res.putpalette(newpal)
    res.putdata(qd)
    return res, warnings


def bake_still(
    rgba: Image.Image,
    tw: int,
    th: int,
    *,
    max_colors: int = 256,
    fit: Literal["letterbox", "stretch"] = "stretch",
    warn_event_layer: bool = False,
) -> tuple[Image.Image, list[str]]:
    """
    RGBA → 8bit P、最大 max_colors 色。index0 は画面上で未使用（マゼンタ予約）。
    """
    warnings: list[str] = []
    if tw % 8 != 0 or th % 8 != 0:
        warnings.append("幅・高さは 8 の倍数にすることを推奨します（タイル境界）。")
    if (tw, th) != STILL_DEFAULT_SIZE:
        warnings.append(
            f"スチルは通常 {STILL_DEFAULT_SIZE[0]}×{STILL_DEFAULT_SIZE[1]} です（現在 {tw}×{th}）。"
        )

    if fit == "stretch":
        work = _stretch_rgba(rgba, tw, th)
    else:
        work = _letterbox_rgba(rgba, tw, th)

    # アルファは白に合成（背景用）
    r, g, b, a = work.split()
    base = Image.new("RGB", work.size, (255, 255, 255))
    base.paste(Image.merge("RGB", (r, g, b)), mask=a)

    mc = max(2, min(256, int(max_colors)))
    q = base.quantize(colors=mc, method=Image.MEDIANCUT, dither=Image.NONE)
    pal = q.getpalette()
    if not pal:
        pal = [0] * (256 * 3)
    else:
        pal = pal[: min(len(pal), 256 * 3)]
        if len(pal) < 256 * 3:
            pal += [0] * (256 * 3 - len(pal))

    qd = list(q.getdata())
    counts = Counter(qd)

    # index0 を未使用に: データ上 0 を最小出現の別インデックスへ寄せる
    if counts[0] > 0:
        candidates = [i for i in range(1, 256) if counts[i] == 0]
        if candidates:
            rep = candidates[0]
            qd = [rep if x == 0 else x for x in qd]
        else:
            rep = min(range(1, 256), key=lambda i: counts.get(i, 0))
            warnings.append(
                "パレットが一杯のため index0 のクリアに他色への寄せが発生しました（ごく僅かな色ズレの可能性）。"
            )
            qd = [rep if x == 0 else x for x in qd]

    pal[0:3] = 255, 0, 255

    res = Image.new("P", (tw, th))
    res.putpalette(pal)
    res.putdata(qd)

    n_after = len({x for x in qd})
    if warn_event_layer and n_after > STILL_EVENT_BG_MAX_COLORS:
        warnings.append(
            f"使用インデックス数が {n_after} です。イベント奥レイヤー用なら {STILL_EVENT_BG_MAX_COLORS} 色以内を推奨（+UI16色で256上限）。"
        )
    if n_after > 240 and not warn_event_layer:
        warnings.append(
            f"色数が多めです（{n_after}）。他BGと併用する画面では240色以内の検討を。"
        )

    return res, warnings


def analyze_indexed_bmp(path: str) -> dict:
    """8bit BMP を読み、検証用の統計を返す。"""
    im = Image.open(path)
    im.load()
    if im.mode != "P":
        return {"error": f"モードが P ではありません: {im.mode}"}
    w, h = im.size
    data = list(im.getdata())
    counts = Counter(data)
    used_idx = sorted(counts.keys())
    n_distinct_indices = len(used_idx)
    pal = im.getpalette() or []
    idx0_px = counts.get(0, 0)
    max_idx = max(used_idx) if used_idx else 0
    return {
        "width": w,
        "height": h,
        "distinct_index_count": n_distinct_indices,
        "max_index": max_idx,
        "index0_pixel_count": idx0_px,
        "total_pixels": w * h,
    }


def validate_sprite_bmp(path: str) -> list[str]:
    wmsgs: list[str] = []
    if not os.path.isfile(path):
        return [f"ファイルがありません: {path}"]
    info = analyze_indexed_bmp(path)
    if "error" in info:
        return [info["error"]]
    w, h = info["width"], info["height"]
    if (w, h) not in ALLOWED_SPRITE_SIZES:
        wmsgs.append(f"[スプライト] サイズ {w}×{h} は規定外です。")
    if info["index0_pixel_count"] > 0:
        wmsgs.append(
            f"[スプライト] インデックス0のピクセルが {info['index0_pixel_count']} 個あります（透過=idx0 推奨）。"
        )
    if info["distinct_index_count"] > 16:
        wmsgs.append(
            f"[スプライト] 画素で使われているパレット番号が {info['distinct_index_count']} 種（16色以内にしてください）。"
        )
    if info["max_index"] > 15:
        wmsgs.append(
            f"[スプライト] 最大インデックスが {info['max_index']} です（通常 0〜15）。"
        )
    return wmsgs


def validate_still_bmp(path: str, *, event_layer: bool = False) -> list[str]:
    wmsgs: list[str] = []
    if not os.path.isfile(path):
        return [f"ファイルがありません: {path}"]
    info = analyze_indexed_bmp(path)
    if "error" in info:
        return [info["error"]]
    w, h = info["width"], info["height"]
    if (w, h) != STILL_DEFAULT_SIZE:
        wmsgs.append(f"[スチル] 推奨は {STILL_DEFAULT_SIZE[0]}×{STILL_DEFAULT_SIZE[1]} です（現在 {w}×{h}）。")
    if info["index0_pixel_count"] > 0:
        wmsgs.append(
            f"[スチル] インデックス0が {info['index0_pixel_count']} ピクセル使われています（絵に使わない推奨）。"
        )
    n = info["distinct_index_count"]
    lim = STILL_EVENT_BG_MAX_COLORS if event_layer else 256
    if n > lim:
        wmsgs.append(f"[スチル] 画素で使う色数が多いです（{n} / 推奨上限 {lim}）。")
    return wmsgs


def save_bmp(im_p: Image.Image, out_path: str) -> None:
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    im_p.save(out_path)
