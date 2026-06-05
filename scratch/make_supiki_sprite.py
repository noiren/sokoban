import os
from PIL import Image, ImageSequence

def remove_bg(img):
    img = img.convert("RGBA")
    data = img.getdata()
    new_data = []
    # Assume top-left pixel is bg color
    bg_color = data[0]
    
    # Simple threshold for background removal (if it's close to bg color)
    for item in data:
        if abs(item[0]-bg_color[0]) < 10 and abs(item[1]-bg_color[1]) < 10 and abs(item[2]-bg_color[2]) < 10:
            new_data.append((255, 0, 255, 0)) # Magenta transparent
        elif item[3] < 128:
            new_data.append((255, 0, 255, 0)) # Transparent
        else:
            new_data.append((item[0], item[1], item[2], 255))
    img.putdata(new_data)
    return img

def create_sprite():
    base_dir = os.path.dirname(os.path.dirname(__file__))
    png_dir = os.path.join(base_dir, "Asset", "graphics", "png")
    out_path = os.path.join(base_dir, "Asset", "graphics", "sprites", "spr_player.bmp")
    
    front_path = os.path.join(png_dir, "supiki_front.webp")
    back_path = os.path.join(png_dir, "supiki_back.jpg")
    
    if not os.path.exists(front_path) or not os.path.exists(back_path):
        print("Images not found!")
        return

    from PIL import ImageOps

    front_img = remove_bg(Image.open(front_path))
    back_img = remove_bg(Image.open(back_path))
    
    # Scale and center-crop to exactly 16x22 so the back image doesn't become too small
    front_img = ImageOps.fit(front_img, (16, 22), method=Image.Resampling.LANCZOS)
    back_img = ImageOps.fit(back_img, (16, 22), method=Image.Resampling.LANCZOS)
    
    # We need 12 frames of 16x32.
    sheet = Image.new('RGBA', (16, 384), (255, 0, 255, 255)) # Magenta bg
    
    # Reverse left and right (front_img naturally faces right)
    left_img = front_img.transpose(Image.FLIP_LEFT_RIGHT)
    right_img = front_img
    
    dir_images = [front_img, back_img, left_img, right_img]
    
    for dir_idx in range(4):
        base_img = dir_images[dir_idx]
        bw, bh = base_img.size
        # Center horizontally
        ox = (16 - bw) // 2
        
        for frame_idx in range(3):
            base_y = (dir_idx * 3 + frame_idx) * 32
            
            # Base y offset (bottom aligned to y=30)
            oy = 30 - bh
            
            # Animation bounce
            if frame_idx == 1:
                oy -= 1 # jump up
            elif frame_idx == 2:
                oy += 1 # squat down
                
            # Paste
            sheet.paste(base_img, (ox, base_y + oy), base_img)

    # Convert to 8bpp (256 colors) with palette
    sheet_p = sheet.quantize(colors=255, method=Image.Quantize.FASTOCTREE)
    
    # Make sure magenta (255, 0, 255) is at index 0
    palette = sheet_p.getpalette()
    
    # Find magenta in palette
    magenta_idx = 0
    for i in range(255):
        if palette[i*3] == 255 and palette[i*3+1] == 0 and palette[i*3+2] == 255:
            magenta_idx = i
            break
            
    # Swap index 0 and magenta_idx
    if magenta_idx != 0:
        # swap palette
        p0 = palette[0:3]
        pm = palette[magenta_idx*3:magenta_idx*3+3]
        palette[0:3] = pm
        palette[magenta_idx*3:magenta_idx*3+3] = p0
        sheet_p.putpalette(palette)
        
        # swap pixel data
        data = list(sheet_p.getdata())
        new_data = []
        for p in data:
            if p == 0:
                new_data.append(magenta_idx)
            elif p == magenta_idx:
                new_data.append(0)
            else:
                new_data.append(p)
        sheet_p.putdata(new_data)
        
    sheet_p.save(out_path)
    print(f"Generated spritesheet at {out_path}")

if __name__ == "__main__":
    create_sprite()
