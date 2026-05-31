import os
import json
from PIL import Image, ImageDraw, ImageFont

# パス設定
WORK_DIR = r"C:\Users\ryota\.gemini\antigravity\scratch\gba-sokoban-bn"
TTF_PATH = r"C:\Users\ryota\Downloads\PixelMplus-20130602\PixelMplus-20130602\PixelMplus12-Bold.ttf"
DOC_PATH = r"C:\Users\ryota\Documents\Obsidian Vault\ｽﾋﾟｷとマヨの倉庫番\概要.md"

BMP_OUT = os.path.join(WORK_DIR, "Asset", "graphics", "sprites", "ui", "japanese_font.bmp")
JSON_OUT = os.path.join(WORK_DIR, "Asset", "graphics", "sprites", "ui", "japanese_font.json")
HEADER_OUT = os.path.join(WORK_DIR, "src", "game", "include", "japanese_sprite_font.h")

def main():
    if not os.path.exists(TTF_PATH):
        print(f"Error: Font not found at {TTF_PATH}")
        return

    # 1. 必要な文字セットの収集
    chars = set()
    # ASCII (33-126) ※ButanoはSpace(32)を展開せず、! (33) から開始する(94文字)
    for c in range(33, 127):
        chars.add(chr(c))
    
    # 概要.mdからの文字抽出
    if os.path.exists(DOC_PATH):
        with open(DOC_PATH, 'r', encoding='utf-8') as f:
            for c in f.read():
                if ord(c) > 32:  # 制御文字・スペース除外
                    chars.add(c)
                    
    # src/ 以下の全ての自動スキャン
    import glob
    src_dir = os.path.join(WORK_DIR, "src")
    for file_path in glob.glob(src_dir + "/**/*.cpp", recursive=True) + glob.glob(src_dir + "/**/*.h", recursive=True):
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            for c in f.read():
                if ord(c) > 32: # スペース以上を除外
                    chars.add(c)
                    
    # Asset/fixdata/ 以下のJSONの自動スキャン
    fixdata_dir = os.path.join(WORK_DIR, "Asset", "fixdata")
    for file_path in glob.glob(fixdata_dir + "/**/*.json", recursive=True):
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            for c in f.read():
                if ord(c) > 32:
                    chars.add(c)
    
    # UIなどで使う特定の文字列（念のため）
    extra_text = "セーブデータを選んでくださいこの作品は同人作品ですオートセーブを使用します上書きされることがストーリーモードプラクティスエンドレスギャラリー設定"
    for c in extra_text:
        if ord(c) > 32:
            chars.add(c)
        
    if ' ' in chars:
        chars.remove(' ')

    # ASCII(94文字)を最初にする必要がある
    ascii_chars = [chr(c) for c in range(33, 127)]
    utf8_chars = sorted([c for c in chars if ord(c) > 127])
    
    all_chars = ascii_chars + utf8_chars
    num_chars = len(all_chars)
    print(f"Total ASCII chars: {len(ascii_chars)}")
    print(f"Total UTF-8 chars: {len(utf8_chars)}")
    print(f"Total characters: {num_chars}")

    # 2. 画像の生成
    # GBAのパレット(16色)に合わせるため、グレースケールで描画して後で変換
    font = ImageFont.truetype(TTF_PATH, 12)
    # ※Butanoのspriteの要件：widthは16の倍数だが、縦1列に並べる場合はwidth=16, height=16*num_chars
    img_w = 16
    img_h = 16 * num_chars
    
    # Butano はマゼンタ (255, 0, 255) または (0, 255, 0) を透過色にするか、
    # 0番インデックスを透過色にする。BMP(8bit/4bit)のパレット0番が背景になる。
    # RGBモードで描画し、後でパレットモード(P)に変換する。
    # 透過色: マゼンタ (FF00FF), 文字色: 白 (FFFFFF)
    img = Image.new('RGB', (img_w, img_h), (255, 0, 255))
    draw = ImageDraw.Draw(img)

    widths = [6] # Space(32)の幅を先頭に追加（Butanoの仕様上、グラフィックは94個だがwidthsは+1個必要）

    for i, c in enumerate(all_chars):
        y_offset = i * 16

        # 文字の描画サイズを取得
        # pillow<10 の場合は textsize、10以降は textbbox 等を使う
        if hasattr(draw, 'textbbox'):
            bbox = draw.textbbox((0, 0), c, font=font)
            w = bbox[2] - bbox[0]
        else:
            w, _ = draw.textsize(c, font=font)
            
        # 最低幅の設定（スペース等）
        if c == ' ':
            w = 6
        elif w == 0:
            w = 8
            
        # 余白を持たせる
        w = min(img_w, w + 1)
        widths.append(w)

        # 描画 (16x16のマスにセンタリング気味に.. 左寄せでも良い)
        # Y=1くらいに描画すると12pxフォントが16pxの中で大体真ん中になる
        draw.text((1, y_offset + 1), c, font=font, fill=(255, 255, 255))

    # パレット画像へ変換
    # 確実にパレットのインデックス0をマゼンタ、最後の方を白にする
    pal_img = Image.new('P', (1, 1))
    palette = [255, 0, 255,   255, 255, 255] # 0 = Magenta, 1 = White
    # 残りを黒で埋める
    while len(palette) < 256 * 3:
        palette.extend([0, 0, 0])
    pal_img.putpalette(palette)
    
    img = img.quantize(palette=pal_img, dither=0)
    
    img.save(BMP_OUT)
    print(f"Saved: {BMP_OUT}")

    # 3. JSONの生成 (height=16を指定して16x16のスプライトとして認識させる)
    json_data = {
        "type": "sprite",
        "height": 16
    }
    with open(JSON_OUT, 'w', encoding='utf-8') as f:
        json.dump(json_data, f, indent=4)
    print(f"Saved: {JSON_OUT}")

    # 4. ヘッダの生成
    # C++ Header string
    cpp = f"""#ifndef JAPANESE_SPRITE_FONT_H
#define JAPANESE_SPRITE_FONT_H

#include "bn_sprite_font.h"
#include "bn_utf8_characters_map.h"
#include "bn_sprite_items_japanese_font.h"
#include "bn_span.h"

constexpr bn::utf8_character japanese_font_utf8_characters[] = {{
"""
    # utf8文字リストを追加 (5個ずつ改行)
    for i in range(0, len(utf8_chars), 5):
        chunk = utf8_chars[i:i+5]
        line = "    " + ", ".join([f'"{c}"' for c in chunk]) + ","
        cpp += line + "\n"

    cpp += f"""}};

constexpr bn::span<const bn::utf8_character> japanese_font_utf8_characters_span(
        japanese_font_utf8_characters);

constexpr auto japanese_font_utf8_characters_map =
        bn::utf8_characters_map<japanese_font_utf8_characters_span>();

constexpr int8_t japanese_font_widths[] = {{
"""
    # widthリストを追加 (16個ずつ改行)
    for i in range(0, len(widths), 16):
        chunk = widths[i:i+16]
        line = "    " + ", ".join([str(w) for w in chunk]) + ","
        cpp += line + "\n"

    cpp += f"""}};

constexpr bn::span<const int8_t> japanese_font_widths_span(japanese_font_widths);

constexpr bn::sprite_font japanese_sprite_font(
        bn::sprite_items::japanese_font,
        japanese_font_utf8_characters_map.reference(),
        japanese_font_widths_span
);

#endif // JAPANESE_SPRITE_FONT_H
"""
    
    with open(HEADER_OUT, 'w', encoding='utf-8') as f:
        f.write(cpp)
    print(f"Saved: {HEADER_OUT}")

if __name__ == "__main__":
    main()
