import struct
import os

def create_indexed_bmp(filename, width, height, pixels, palette):
    # Ensure palette has exactly 256 entries for 8-bit mapping
    palette_data = palette + b'\x00\x00\x00\x00' * (256 - len(palette)//4)

    row_bytes = width
    padding = (4 - (row_bytes % 4)) % 4
    
    # BMP stored bottom-up
    pixel_data = b''.join([bytes(pixels[y*width:(y+1)*width]) + b'\x00'*padding for y in range(height-1, -1, -1)])
    
    header = struct.pack('<2sIHHI', b'BM', 54 + 1024 + len(pixel_data), 0, 0, 54 + 1024)
    info = struct.pack('<IiiHHIIIIII', 40, width, height, 1, 8, 0, len(pixel_data), 2835, 2835, 256, 256)
    
    with open(filename, 'wb') as f:
        f.write(header + info + palette_data + pixel_data)

# Palette (BGR0 format)
# Index 0: Transparent (Black here, but engine treats 0 as transparent)
# Index 1: Beige (Paper base)   #D2B48C -> BGR: 140, 180, 210 -> \x8c\xb4\xd2\x00
# Index 2: Brown (Border)       #8B4513 -> BGR:  19,  69, 139 -> \x13\x45\x8b\x00
# Index 3: Blue (Icon base 1)   #4169E1 -> BGR: 225, 105,  65 -> \xe1\x69\x41\x00
# Index 4: Yellow (Icon base 2) #FFD700 -> BGR:   0, 215, 255 -> \x00\xd7\xff\x00

pal = (
    b'\x00\x00\x00\x00' + # 0: Transp
    b'\x8c\xb4\xd2\x00' + # 1: Beige
    b'\x13\x45\x8b\x00' + # 2: Brown
    b'\xe1\x69\x41\x00' + # 3: Blue
    b'\x00\xd7\xff\x00'   # 4: Yellow
)

# 1. Paper Sprite (64x64)
# A rectangle with a 2-pixel brown border and beige inner.
p_w, p_h = 64, 64
pixels_paper = [0] * (p_w * p_h)
for y in range(p_h):
    for x in range(p_w):
        # simple border logic (rounded corner simulation)
        if (x < 2 or x > p_w-3 or y < 2 or y > p_h-3):
            pixels_paper[y * p_w + x] = 2 # Brown
        else:
            pixels_paper[y * p_w + x] = 1 # Beige

create_indexed_bmp('graphics/spr_menu_paper.bmp', p_w, p_h, pixels_paper, pal)

# 2. Icon Sprite (32x32)
# A simple circle-ish shape
i_w, i_h = 32, 32
pixels_icon1 = [0] * (i_w * i_h)
pixels_icon2 = [0] * (i_w * i_h)

for y in range(i_h):
    for x in range(i_w):
        cx, cy = 16, 16
        dist_sq = (x-cx)**2 + (y-cy)**2
        if dist_sq < 14**2:
            pixels_icon1[y * i_w + x] = 3 # Blue
            pixels_icon2[y * i_w + x] = 4 # Yellow

create_indexed_bmp('graphics/spr_menu_icon_story.bmp', i_w, i_h, pixels_icon1, pal)
create_indexed_bmp('graphics/spr_menu_icon_practice.bmp', i_w, i_h, pixels_icon2, pal)

print("Generated dummy BMPs successfully!")
