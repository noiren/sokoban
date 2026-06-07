#!/usr/bin/env python3
"""
仮(プレースホルダ)UIテクスチャ生成ツール。

灰色ベタのダミーBMPを「それっぽい仮絵」に差し替える。
将来、同じパス・同じサイズに本番BMPを置けばそのまま入れ替わる前提。

GBA制約:
  - 8bit インデックスカラー BMP
  - 使用色は 16 色以内(マゼンタ含む)
  - パレット index0 = マゼンタ(#FF00FF) = 透過

実行: python src/tools/gen_temp_placeholders.py
"""
import os
from PIL import Image, ImageDraw

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
GFX = os.path.join(PROJECT_ROOT, "Asset", "graphics", "sprites")

# 共有パレット (index: RGB)。index0 は透過マゼンタ固定。
PAL = [
    (255, 0, 255),    # 0 透過
    (74, 52, 33),     # 1 こげ茶(枠線)
    (236, 214, 165),  # 2 紙ベージュ
    (205, 178, 124),  # 3 紙の陰
    (255, 218, 92),   # 4 金(明)
    (190, 140, 30),   # 5 金(暗)
    (250, 248, 240),  # 6 白
    (54, 40, 30),     # 7 輪郭(濃)
    (214, 78, 60),    # 8 赤アクセント
    (74, 120, 200),   # 9 青アクセント
]


def new_canvas(w, h):
    img = Image.new("P", (w, h), 0)
    flat = []
    for rgb in PAL:
        flat += list(rgb)
    flat += [0, 0, 0] * (256 - len(PAL))
    img.putpalette(flat)
    return img


def save(img, rel_path):
    out = os.path.join(GFX, rel_path)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    img.save(out)
    print(f"  wrote {rel_path}")


def round_corners(img, r=2):
    """四隅 r x r の三角を透過(0)にして角を落とす。"""
    w, h = img.size
    px = img.load()
    for dy in range(r):
        for dx in range(r - dy):
            px[dx, dy] = 0                       # 左上
            px[w - 1 - dx, dy] = 0               # 右上
            px[dx, h - 1 - dy] = 0               # 左下
            px[w - 1 - dx, h - 1 - dy] = 0       # 右下
    return img


def make_paper(w, h, pad):
    """中央に紙パネルを描く (64x64キャンバス, pad で内側余白を調整)。"""
    img = new_canvas(w, h)
    d = ImageDraw.Draw(img)
    x0, y0, x1, y1 = pad, pad, w - 1 - pad, h - 1 - pad
    # 影(右下に1px落とす)
    d.rectangle([x0 + 1, y0 + 1, x1 + 1, y1 + 1], fill=3)
    # 枠
    d.rectangle([x0, y0, x1, y1], fill=1)
    # 紙面
    d.rectangle([x0 + 2, y0 + 2, x1 - 2, y1 - 2], fill=2)
    # 上端ハイライト
    d.line([x0 + 3, y0 + 3, x1 - 3, y0 + 3], fill=6)
    round_corners(img, 2)
    return img


def make_paper_tail():
    """吹き出しの尻尾 (64x64中央に下向き三角)。"""
    img = new_canvas(64, 64)
    d = ImageDraw.Draw(img)
    cx = 32
    top = 16
    bottom = 46
    half = 12
    d.polygon([(cx - half, top), (cx + half, top), (cx, bottom)], fill=1)
    d.polygon([(cx - half + 2, top + 1), (cx + half - 2, top + 1), (cx, bottom - 3)], fill=2)
    return img


def make_corner_bracket():
    """金枠の左上(TL)パーツ (32x32)。L字。他の隅は反転で生成。"""
    img = new_canvas(32, 32)
    d = ImageDraw.Draw(img)
    ln = 16   # 腕の長さ
    th = 4    # 太さ
    # 横棒
    d.rectangle([0, 0, ln, th - 1], fill=5)
    d.rectangle([0, 0, ln, th - 3], fill=4)
    # 縦棒
    d.rectangle([0, 0, th - 1, ln], fill=5)
    d.rectangle([0, 0, th - 3, ln], fill=4)
    return img


def make_cursor():
    """汎用カーソル(右向き矢印ポインタ) 32x32。"""
    img = new_canvas(32, 32)
    d = ImageDraw.Draw(img)
    # 輪郭付きの右向き三角
    d.polygon([(8, 6), (24, 16), (8, 26)], fill=7)
    d.polygon([(10, 10), (20, 16), (10, 22)], fill=6)
    d.polygon([(11, 12), (17, 16), (11, 20)], fill=8)
    return img


def make_sparkle():
    """キラキラ(4芒星) 32x32。"""
    img = new_canvas(32, 32)
    d = ImageDraw.Draw(img)
    cx, cy = 16, 16
    # 縦横の光条
    d.polygon([(cx, 2), (cx + 3, cy), (cx, 30), (cx - 3, cy)], fill=4)
    d.polygon([(2, cy), (cx, cy - 3), (30, cy), (cx, cy + 3)], fill=4)
    d.ellipse([cx - 3, cy - 3, cx + 3, cy + 3], fill=6)
    return img


def make_button_ok():
    """Aボタンガイド(丸+A) 32x32。"""
    img = new_canvas(32, 32)
    d = ImageDraw.Draw(img)
    d.ellipse([3, 3, 28, 28], fill=5)
    d.ellipse([5, 5, 26, 26], fill=4)
    d.ellipse([7, 7, 24, 24], fill=2)
    # 文字 A (線で描画)
    d.line([(16, 10), (11, 22)], fill=7)
    d.line([(16, 10), (21, 22)], fill=7)
    d.line([(13, 18), (19, 18)], fill=7)
    return img


def make_nav_arrow():
    """ページ送り矢印(右向き三角) 32x32。"""
    img = new_canvas(32, 32)
    d = ImageDraw.Draw(img)
    d.polygon([(10, 6), (24, 16), (10, 26)], fill=5)
    d.polygon([(12, 10), (20, 16), (12, 22)], fill=4)
    return img


def make_photo_corner():
    """写真の角留め(左上の三角マウント) 32x32。"""
    img = new_canvas(32, 32)
    d = ImageDraw.Draw(img)
    d.polygon([(0, 0), (18, 0), (0, 18)], fill=1)
    d.polygon([(0, 0), (13, 0), (0, 13)], fill=7)
    return img


def flip_lr(img):
    return img.transpose(Image.FLIP_LEFT_RIGHT)


def flip_tb(img):
    return img.transpose(Image.FLIP_TOP_BOTTOM)


def main():
    print("[gen_temp_placeholders] generating ui/paper ...")
    save(make_paper(64, 64, 2), "ui/paper/spr_paper_large.bmp")
    save(make_paper(64, 64, 9), "ui/paper/spr_paper_medium.bmp")
    save(make_paper(64, 64, 16), "ui/paper/spr_paper_small.bmp")
    save(make_paper_tail(), "ui/paper/spr_paper_tail.bmp")

    print("[gen_temp_placeholders] generating ui/common ...")
    save(make_cursor(), "ui/common/spr_selection_common.bmp")
    tl = make_corner_bracket()
    save(tl, "ui/common/spr_selector_gold_tl.bmp")
    save(flip_lr(tl), "ui/common/spr_selector_gold_tr.bmp")
    save(flip_tb(tl), "ui/common/spr_selector_gold_bl.bmp")
    save(flip_tb(flip_lr(tl)), "ui/common/spr_selector_gold_br.bmp")
    save(make_sparkle(), "ui/common/spr_sparkle.bmp")
    save(make_button_ok(), "ui/common/spr_button_ok.bmp")
    save(make_nav_arrow(), "ui/common/spr_nav_arrow.bmp")
    save(make_photo_corner(), "ui/common/spr_photo_corner.bmp")

    print("[gen_temp_placeholders] done.")


if __name__ == "__main__":
    main()
