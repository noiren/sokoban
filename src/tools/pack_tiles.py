import os
import io
import re
from PIL import Image

def pack():
    # フォルダパス
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(os.path.dirname(script_dir))
    tiles_dir = os.path.join(project_root, "Asset", "graphics", "tiles")
    bmp_path = os.path.join(project_root, "Asset", "graphics", "stills", "still_puzzle_map.bmp")
    
    if not os.path.exists(tiles_dir):
        print(f"[pack_tiles] Error: {tiles_dir} not found. Skipping packing.")
        return False
        
    # タイル画像の検索
    tile_files = []
    for file in os.listdir(tiles_dir):
        if file.endswith(".bmp"):
            match = re.match(r"^(\d+)_(.+)\.bmp$", file)
            if match:
                tile_id = int(match.group(1))
                tile_files.append((tile_id, os.path.join(tiles_dir, file)))
                
    if not tile_files:
        print("[pack_tiles] No tile files found in Asset/graphics/tiles/. Skipping.")
        return False
        
    # 基準となるパレットをロードする。
    # 既存の still_puzzle_map.bmp があればそのパレットを流用。
    # なければ、最初のタイル画像のパレットを流用。
    palette = None
    if os.path.exists(bmp_path):
        try:
            with open(bmp_path, "rb") as f:
                img_data = f.read()
            with Image.open(io.BytesIO(img_data)) as img:
                img_p = img.convert("P")
                palette = img_p.getpalette()
        except Exception as e:
            print(f"[pack_tiles] Warning: Failed to load existing palette ({e})")
            
    # パレットが取得できなかった場合のフォールバック（初期パレット生成）
    if not palette:
        try:
            first_tile_path = tile_files[0][1]
            with open(first_tile_path, "rb") as f:
                img_data = f.read()
            with Image.open(io.BytesIO(img_data)) as img:
                img_p = img.convert("P")
                palette = img_p.getpalette()
        except Exception as e:
            print(f"[pack_tiles] Warning: Failed to load tile palette ({e})")
            # GBA既定のマゼンタを0番目にしたダミー
            palette = [255, 0, 255] + [0, 0, 0] * 255
            
    # 512x512 ピクセルの透過（インデックス0）で埋めたバッファ画像を用意
    pixels = [0] * (512 * 512)
    
    # 既存の still_puzzle_map.bmp をロードして、新しく上書きしない部分を維持する
    if os.path.exists(bmp_path):
        try:
            with open(bmp_path, "rb") as f:
                img_data = f.read()
            with Image.open(io.BytesIO(img_data)) as img:
                img_p = img.convert("P")
                pixels = list(img_p.getdata())
                # もし既存の画像が512x512じゃなければ作り直すためにパディング等の処理が必要だが、一旦簡略化
                if len(pixels) != 512*512:
                    pixels = [0] * (512 * 512)
        except Exception as e:
            print(f"[pack_tiles] Warning: Failed to load existing cells ({e})")
            
    # 各個別タイルをバッファに書き込む
    for tile_id, path in tile_files:
        try:
            with open(path, "rb") as f:
                img_data = f.read()
            with Image.open(io.BytesIO(img_data)) as img:
                img_p = img.convert("P")
                # 16x16であることを検証
                if img_p.size != (16, 16):
                    print(f"[pack_tiles] Warning: Tile {path} is not 16x16 (size: {img_p.size}). Skipping.")
                    continue
                
                tile_pixels = list(img_p.getdata())
                
                # タイルの座標
                tile_x = (tile_id % 16) * 16
                tile_y = (tile_id // 16) * 16
                
                for dy in range(16):
                    for dx in range(16):
                        val = tile_pixels[dy * 16 + dx]
                        pixels[(tile_y + dy) * 512 + (tile_x + dx)] = val
        except Exception as e:
            print(f"[pack_tiles] Error: Failed to process tile {path} ({e})")
            
    # 新しいピクセルデータで画像を生成して保存
    new_img = Image.new("P", (512, 512))
    new_img.putpalette(palette)
    new_img.putdata(pixels)
    
    # 保存 (Windows のロック問題を回避するために一時ファイルを経由)
    os.makedirs(os.path.dirname(bmp_path), exist_ok=True)
    temp_path = os.path.join(os.path.dirname(bmp_path), "still_puzzle_map_pack_temp.bmp")
    
    try:
        new_img.save(temp_path)
        if os.path.exists(bmp_path):
            os.remove(bmp_path)
        os.rename(temp_path, bmp_path)
        print(f"[pack_tiles] Successfully packed all individual tiles into {bmp_path}!")
        return True
    except Exception as e:
        print(f"[pack_tiles] Error: Failed to save packed image ({e})")
        if os.path.exists(temp_path):
            try: os.remove(temp_path)
            except: pass
        return False

def main():
    pack()

if __name__ == "__main__":
    main()
