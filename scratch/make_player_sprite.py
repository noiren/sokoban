import os
from PIL import Image, ImageDraw

def create_sprite():
    w, h = 16, 32
    frames = 12
    img = Image.new('P', (w, h * frames))
    
    # Simple palette
    palette = [
        255, 0, 255,   # 0: Transparent (Magenta)
        0, 0, 0,       # 1: Outline
        200, 200, 200, # 2: Skin
        255, 0, 0,     # 3: Down
        0, 0, 255,     # 4: Up
        0, 255, 0,     # 5: Left
        255, 255, 0,   # 6: Right
        100, 100, 100, # 7: Shadow/Legs
    ]
    # pad palette to 256
    palette += [0] * (768 - len(palette))
    img.putpalette(palette)
    
    draw = ImageDraw.Draw(img)
    draw.rectangle([0, 0, w-1, (h * frames)-1], fill=0) # transparent bg
    
    directions = [3, 4, 5, 6] # Down, Up, Left, Right
    
    for dir_idx, color in enumerate(directions):
        for frame_idx in range(3):
            # Frame 0: Stand, Frame 1: Left leg, Frame 2: Right leg
            base_y = (dir_idx * 3 + frame_idx) * h
            
            # Head (offset slightly so it spans y=8 to y=15 approx, meaning 16x24 overall size)
            # Body fits within 16x16 tile area (y=16 to 31)
            # Let's draw head at 2-13, body at 14-25, legs at 26-31
            draw.ellipse([4, base_y + 4, 11, base_y + 13], fill=2, outline=1)
            
            # Body
            draw.rectangle([4, base_y + 14, 11, base_y + 24], fill=color, outline=1)
            
            # Legs
            left_leg_y = base_y + 28
            right_leg_y = base_y + 28
            if frame_idx == 1:
                left_leg_y = base_y + 25
            elif frame_idx == 2:
                right_leg_y = base_y + 25
                
            draw.rectangle([4, base_y + 25, 6, left_leg_y], fill=7, outline=1)
            draw.rectangle([9, base_y + 25, 11, right_leg_y], fill=7, outline=1)

    out_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), "Asset", "graphics", "sprites", "spr_player.bmp")
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    img.save(out_path)
    print(f"Created {out_path}")

if __name__ == "__main__":
    create_sprite()
