"""
gba_event.bmp を 240色 (15 パレットバンク×16色) に削減するスクリプト。
残り1バンク (16色) を ui_msg_window 用に確保する。
"""
from PIL import Image
import os

def reduce_gba_event(filepath, max_colors=240):
    print(f"Reducing {os.path.basename(filepath)} to max {max_colors} colors...")
    img = Image.open(filepath)
    print(f"  Original: mode={img.mode}, size={img.size}")

    # RGBに変換して処理
    rgb = img.convert("RGB")

    # 240色に量子化（パレットバンク15個分）
    quantized = rgb.quantize(colors=max_colors, method=Image.Quantize.MEDIANCUT)
    pal = quantized.getpalette()  # 768 entries (256×RGB)
    used_colors = len(set(quantized.getdata()))
    print(f"  After quantize: {used_colors} unique colors used (max {max_colors})")

    # 8-bit BMP として保存（Butano が 8-bit regular_bg として処理）
    # パレットインデックス0を確認（透過色扱いになるため、使われていない色にする）
    q_data = list(quantized.getdata())
    idx0_used = q_data.count(0)
    print(f"  Index 0 used by {idx0_used} pixels")

    if idx0_used > 0:
        # index 0 が実際のピクセルに使われている → 最後のインデックスと交換
        # まず使われていないインデックスを探す
        used_set = set(q_data)
        dummy_idx = None
        for i in range(max_colors - 1, -1, -1):
            if i not in used_set or i == 0:
                if i != 0:
                    dummy_idx = i
                    break
        
        if dummy_idx is None:
            # 全インデックスが使われている→ index 0 の色を黒にして、
            # index 0 のピクセルを index 1 に転送
            print("  Warning: all indices used. Shifting index 0 pixels to index 1.")
            new_data = [1 if p == 0 else p for p in q_data]
            quantized.putdata(new_data)
            # index 0 の色を黒にする
            pal[0], pal[1], pal[2] = 0, 0, 0
            quantized.putpalette(pal)
        else:
            print(f"  Swapping index 0 <-> {dummy_idx}")
            # palette swap
            r0, g0, b0 = pal[0], pal[1], pal[2]
            rd, gd, bd = pal[dummy_idx*3], pal[dummy_idx*3+1], pal[dummy_idx*3+2]
            pal[0], pal[1], pal[2] = rd, gd, bd
            pal[dummy_idx*3], pal[dummy_idx*3+1], pal[dummy_idx*3+2] = r0, g0, b0
            quantized.putpalette(pal)
            # pixel swap
            new_data = []
            for p in q_data:
                if p == 0:
                    new_data.append(dummy_idx)
                elif p == dummy_idx:
                    new_data.append(0)
                else:
                    new_data.append(p)
            quantized.putdata(new_data)

    temp = filepath + ".tmp"
    quantized.save(temp, format="BMP")
    img.close()
    os.replace(temp, filepath)
    print(f"  Saved -> {os.path.basename(filepath)}")

reduce_gba_event("Asset/graphics/stills/event/gba_event.bmp", max_colors=240)
print("Done!")
