import os
import json
from PIL import Image

def convert_to_gba_bg(png_path, bmp_path, json_path):
    print(f"Converting {png_path} -> {bmp_path}")
    try:
        # Open the image
        with Image.open(png_path) as img:
            # Ensure it's 240x160
            img = img.resize((240, 160), Image.Resampling.LANCZOS)
            
            # Create a 256x256 black canvas (Butano requirement)
            canvas = Image.new('RGB', (256, 256), color=(0, 0, 0))
            canvas.paste(img, (0, 0))
            
            # Convert to 256 color indexed (Adaptive palette)
            # GBA uses 8-bit palette for regular BGs.
            img_p = canvas.convert('P', palette=Image.ADAPTIVE, colors=256)
            
            # Save as BMP
            img_p.save(bmp_path, format='BMP')
            
        # Create Butano JSON file
        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump({"type": "regular_bg"}, f, indent=2)
            
        print(f"Success: {bmp_path}")
    except Exception as e:
        print(f"Error converting {png_path}: {e}")

if __name__ == "__main__":
    bgs_to_convert = [
        {
            "src": "graphics/temp_uploads/タイトル/gba_タイトル.png",
            "dst_bmp": "graphics/bg_title_main.bmp",
            "dst_json": "graphics/bg_title_main.json"
        },
        {
            "src": "graphics/temp_uploads/メインメニュー/gba_メインメニュー例.png",
            "dst_bmp": "graphics/bg_menu_main.bmp",
            "dst_json": "graphics/bg_menu_main.json"
        }
    ]
    
    for item in bgs_to_convert:
        if os.path.exists(item["src"]):
            convert_to_gba_bg(item["src"], item["dst_bmp"], item["dst_json"])
        else:
            print(f"Warning: Source not found: {item['src']}")
