import os
import io
from PIL import Image

def main():
    bmp_path = os.path.join("Asset", "graphics", "stills", "still_puzzle_map.bmp")
    out_dir = os.path.join("Asset", "graphics", "tiles")
    os.makedirs(out_dir, exist_ok=True)
    
    if not os.path.exists(bmp_path):
        print(f"Error: {bmp_path} not found.")
        return
        
    with open(bmp_path, "rb") as f:
        img_data = f.read()
        
    with Image.open(io.BytesIO(img_data)) as img:
        img_p = img.convert("P")
        palette = img_p.getpalette()
        
        # タイルの意味のあるファイル名マッピング (ID: name)
        tile_names = {
            0: "floor",
            1: "wall",
            2: "player",
            3: "barrel",
            4: "switch",
            5: "barrel_on_switch",
            6: "shady",
            7: "hole",
            8: "exit",
            9: "ice",
            10: "cracked_1",
            11: "cracked_2",
            12: "arrow_up",
            13: "arrow_down",
            14: "arrow_left",
            15: "arrow_right",
            16: "toggle_switch",
            17: "warp_red_a",
            18: "warp_red_b",
            19: "warp_blue_a",
            20: "warp_blue_b",
            21: "goal_closed",
            22: "goal_open"
        }
        
        for tid, name in tile_names.items():
            # 8x8 タイルの切り出し
            tile_x = (tid % 32) * 8
            tile_y = (tid // 32) * 8
            
            # クロップ
            tile_img = img_p.crop((tile_x, tile_y, tile_x + 8, tile_y + 8))
            
            # 切り出した 8x8 タイルも GBA が読める正しいインデックスカラー (P モード) として保存する
            # パレットを引き継ぐ
            out_img = Image.new("P", (8, 8))
            out_img.putpalette(palette)
            out_img.putdata(list(tile_img.getdata()))
            
            out_path = os.path.join(out_dir, f"{tid:02d}_{name}.bmp")
            
            # 保存
            out_img.save(out_path)
            print(f"Sliced: {out_path}")
            
    print("Successfully sliced all tiles into individual 8x8 files!")

if __name__ == "__main__":
    main()
