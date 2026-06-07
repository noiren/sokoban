#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
立ち絵スプライト(image_id -> bn::sprite_items)のディスパッチ生成。

Asset/graphics/sprites/chara/chara/*.bmp を走査し、
  bmp名:  spr_ch_<image_id>.bmp
  image_id: <bmp名から spr_ch_ を除いたもの>
の対応で、image_id 文字列 / 連番 からスプライトを生成する関数を C++ ヘッダに吐く。

これにより ui_manager は立ち絵の対応表を手書きしなくてよくなり、
PNGを変換してBMPを追加するだけで実機に反映される(感情の増減もコード不要)。

生成先: src/game/include/generated/chara_portraits.gen.h
prebuild から呼ばれる。
"""
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
CHARA_BMP_DIR = os.path.join(PROJECT_ROOT, "Asset", "graphics", "sprites", "chara", "chara")
OUT_PATH = os.path.join(
    PROJECT_ROOT, "src", "game", "include", "generated", "chara_portraits.gen.h"
)

PREFIX = "spr_ch_"


def collect_items():
    """[(image_id, sprite_item_name)] を image_id 昇順で返す。"""
    items = []
    if os.path.isdir(CHARA_BMP_DIR):
        for f in sorted(os.listdir(CHARA_BMP_DIR)):
            if not f.lower().endswith(".bmp"):
                continue
            base = os.path.splitext(f)[0]  # spr_ch_mayo_normal_1
            if not base.startswith(PREFIX):
                continue
            image_id = base[len(PREFIX):]  # mayo_normal_1
            items.append((image_id, base))
    return items


def build_source(items):
    L = []
    L.append("// AUTO GENERATED FILE. DO NOT EDIT. (generate_chara_portraits.py)")
    L.append("#pragma once")
    L.append("")
    L.append('#include "bn_sprite_ptr.h"')
    L.append('#include "bn_optional.h"')
    L.append('#include "bn_string_view.h"')
    L.append('#include "bn_fixed.h"')
    for _, item in items:
        L.append(f'#include "bn_sprite_items_{item}.h"')
    L.append("")
    L.append("namespace chara_portraits {")
    L.append("")
    L.append(f"constexpr int COUNT = {len(items)};")
    L.append("")
    # by index
    L.append("inline bn::optional<bn::sprite_ptr> create_by_index(int index, bn::fixed x, bn::fixed y) {")
    for i, (_, item) in enumerate(items):
        kw = "if" if i == 0 else "else if"
        L.append(f"    {kw} (index == {i}) return bn::sprite_items::{item}.create_sprite(x, y);")
    L.append("    return bn::optional<bn::sprite_ptr>();")
    L.append("}")
    L.append("")
    # by id
    L.append("inline bn::optional<bn::sprite_ptr> create_by_id(const bn::string_view& image_id, bn::fixed x, bn::fixed y) {")
    for i, (image_id, item) in enumerate(items):
        kw = "if" if i == 0 else "else if"
        L.append(f'    {kw} (image_id == "{image_id}") return bn::sprite_items::{item}.create_sprite(x, y);')
    L.append("    return bn::optional<bn::sprite_ptr>();")
    L.append("}")
    L.append("")
    # id -> index (任意で使えるよう)
    L.append("inline int index_of(const bn::string_view& image_id) {")
    for i, (image_id, _) in enumerate(items):
        kw = "if" if i == 0 else "else if"
        L.append(f'    {kw} (image_id == "{image_id}") return {i};')
    L.append("    return -1;")
    L.append("}")
    L.append("")
    L.append("} // namespace chara_portraits")
    L.append("")
    return "\n".join(L)


def main():
    items = collect_items()
    src = build_source(items)
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    existing = ""
    if os.path.exists(OUT_PATH):
        with open(OUT_PATH, "r", encoding="utf-8") as f:
            existing = f.read()
    if existing != src:
        with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as f:
            f.write(src)
        print(f"[generate_chara_portraits] Generated: {OUT_PATH} ({len(items)} portraits)")
    else:
        print(f"[generate_chara_portraits] Up-to-date ({len(items)} portraits)")


if __name__ == "__main__":
    main()
