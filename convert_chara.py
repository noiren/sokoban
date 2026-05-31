import os
import glob
from PIL import Image

def process_image(input_path, output_path):
    img = Image.open(input_path).convert("RGBA")
    
    # Calculate aspect ratio preserving resize and crop to 64x64
    # Focus on the upper part (face) if it's a character portrait
    w, h = img.size
    
    # We want to fill 64x64. Let's make a 64x64 magenta canvas
    canvas = Image.new("RGBA", (64, 64), (255, 0, 255, 255))
    
    # Determine scaling factor to fit width or height
    # Since these are portraits, maybe we scale so the face fits well
    # Let's just resize the whole image to fit within 64x64
    scale = min(64/w, 64/h)
    new_w = int(w * scale)
    new_h = int(h * scale)
    
    resized = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
    
    # Paste onto canvas, centered horizontally, bottom aligned
    x = (64 - new_w) // 2
    y = 64 - new_h
    
    canvas.paste(resized, (x, y), resized)
    
    # Convert to RGB to flatten alpha (magenta background)
    rgb_img = canvas.convert("RGB")
    
    # Create an image with exactly 16 colors, with magenta at index 0
    pal_img = Image.new('P', (1, 1))
    palette = [255, 0, 255] # 0 = Magenta
    # Fill the rest with black
    while len(palette) < 256 * 3:
        palette.extend([0, 0, 0])
    pal_img.putpalette(palette)
    
    # Quantize to 16 colors
    q_img = rgb_img.quantize(colors=16, dither=Image.Dither.NONE)
    
    # Ensure index 0 is magenta
    final_pal = q_img.getpalette()
    # Find magenta in the new palette
    magenta_idx = -1
    for i in range(16):
        if final_pal[i*3] == 255 and final_pal[i*3+1] == 0 and final_pal[i*3+2] == 255:
            magenta_idx = i
            break
            
    if magenta_idx != -1 and magenta_idx != 0:
        # Swap index 0 and magenta_idx
        # We need to remap pixel data
        def remap(p):
            if p == 0: return magenta_idx
            if p == magenta_idx: return 0
            return p
        q_img = q_img.point(remap)
        # Swap in palette
        final_pal[0:3], final_pal[magenta_idx*3:magenta_idx*3+3] = final_pal[magenta_idx*3:magenta_idx*3+3], final_pal[0:3]
        q_img.putpalette(final_pal)
    elif magenta_idx == -1:
        # Force index 0 to be magenta if it was lost
        final_pal[0:3] = [255, 0, 255]
        q_img.putpalette(final_pal)

    q_img.save(output_path, "BMP")
    print(f"Saved {output_path}")

input_dir = "C:/Users/ryota/.gemini/antigravity/brain/bae4e4e1-9167-4271-bcf7-d9e4344a8a71/"
files = sorted(glob.glob(input_dir + "media__1779212*.png"))

out_dir = "Asset/graphics/sprites/chara/chara/"

mapping = [
    "spr_ch_mayo_normal.bmp",
    "spr_ch_mayo_smile.bmp", # 喜
    "spr_ch_mayo_angry.bmp", # 怒
    "spr_ch_mayo_sad.bmp",   # 哀
    "spr_ch_mayo_fun.bmp",   # 楽
]

for i in range(min(len(files), len(mapping))):
    process_image(files[i], os.path.join(out_dir, mapping[i]))
