import struct

width = 512
height = 512

# 16 colors for our palette
# 0: Magenta (Transparent)
# 1: Floor (Light Gray)
# 2: Wall (Dark Gray)
# 3: Player (Blue)
# 4: Barrel (Brown)
# 5: Switch/Goal (Green)
# 6: Switch+Barrel (Dark Green/Brown)
# 7: Shady (Purple)
# 8: Hole (Black)
# We will draw 18 unique tiles. We can just use these colors, and maybe add some pattern to make them unique if needed, but solid colors are unique as long as they are different.
# Wait, 18 tiles means we need 18 unique 8x8 blocks. If some are solid, we only have 16 colors!
# To make 18 unique tiles, we can draw a border or a dot.
# Let's make a function that draws an 8x8 tile with a background color and a text/number or pattern.
# Actually, we can just use 2 colors per tile: a border (color 1) and a fill (different for each).

palette = [
    (255, 0, 255), # 0: Transparent
    (200, 200, 200), # 1: Floor
    (100, 100, 100), # 2: Wall
    (0, 0, 255),     # 3: Player
    (150, 75, 0),    # 4: Barrel
    (0, 255, 0),     # 5: Switch
    (100, 150, 0),   # 6: Switch+Barrel
    (128, 0, 128),   # 7: Shady
    (0, 0, 0),       # 8: Hole
    (255, 255, 0),   # 9: Yellow
    (0, 255, 255),   # 10: Cyan
    (255, 128, 0),   # 11: Orange
    (255, 0, 0),     # 12: Red
    (128, 128, 128), # 13: Gray
    (50, 50, 50),    # 14: Dark Gray
    (255, 255, 255), # 15: White
]

# We need 256 colors in the palette for 8-bit BMP
full_palette = palette + [(0, 0, 0)] * (256 - len(palette))

pixels = [[0 for _ in range(width)] for _ in range(height)]

def draw_tile(tx, ty, bg_col, fg_col, pattern=0):
    for y in range(16):
        for x in range(16):
            # To make sure every tile is unique, we can put its index in binary or just rely on bg_col + pattern
            px = tx * 16 + x
            py = ty * 16 + y
            if x == 0 or x == 15 or y == 0 or y == 15:
                pixels[py][px] = fg_col
            else:
                pixels[py][px] = bg_col
                if pattern > 0 and (x + y) % pattern == 0:
                    pixels[py][px] = fg_col
            # Embed the tile index (tx) in the pixels so it is mathematically guaranteed to be unique
            if y == 1:
                if (tx & (1 << x)) != 0:
                    pixels[py][px] = 15 # White dot

# 0: Floor
draw_tile(0, 0, 1, 14, 0)
# 1: Wall
draw_tile(1, 0, 2, 14, 0)
# 2: Player
draw_tile(2, 0, 3, 15, 0)
# 3: Barrel
draw_tile(3, 0, 4, 11, 0)
# 4: Goal/Switch
draw_tile(4, 0, 5, 9, 0)
# 5: Switch+Barrel
draw_tile(5, 0, 6, 4, 0)
# 6: Shady
draw_tile(6, 0, 7, 12, 0)
# 7: Hole
draw_tile(7, 0, 8, 2, 0)

# Fill 8 to 16 with dummy unique tiles so that we reach 17 safely
for i in range(8, 17):
    draw_tile(i, 0, 13, 15, i%4 + 1)

# Tile 17 is EMPTY (Transparent)
for y in range(16):
    for x in range(16):
        pixels[0][17 * 16 + x] = 0 # Transparent

# The rest of the image is transparent (index 0)
# Since the array is initialized with 0, we don't need to do anything.

# Write BMP
with open('Asset/graphics/stills/still_puzzle_map.bmp', 'wb') as f:
    # BITMAPFILEHEADER
    f.write(b'BM')
    f.write(struct.pack('<I', 14 + 40 + 256*4 + width*height))
    f.write(b'\x00\x00\x00\x00')
    f.write(struct.pack('<I', 14 + 40 + 256*4))
    
    # BITMAPINFOHEADER
    f.write(struct.pack('<I', 40))
    f.write(struct.pack('<I', width))
    f.write(struct.pack('<I', height))
    f.write(struct.pack('<H', 1)) # planes
    f.write(struct.pack('<H', 8)) # bits per pixel
    f.write(struct.pack('<I', 0)) # compression
    f.write(struct.pack('<I', width * height)) # image size
    f.write(struct.pack('<I', 2835)) # x res
    f.write(struct.pack('<I', 2835)) # y res
    f.write(struct.pack('<I', 256)) # colors
    f.write(struct.pack('<I', 0)) # important colors
    
    # Palette
    for c in full_palette:
        f.write(struct.pack('<BBBB', c[2], c[1], c[0], 0)) # B, G, R, 0
        
    # Pixels (bottom-up)
    for y in range(height-1, -1, -1):
        row = bytes(pixels[y])
        f.write(row)
        # padding to 4 bytes (width=256 is already multiple of 4)

print("Generated Asset/graphics/stills/still_puzzle_map.bmp")
