import os
from PIL import Image

def analyze_image(path):
    print(f'\n--- Analyzing {os.path.basename(path)} ---')
    if not os.path.exists(path):
        print('File not found')
        return
    
    img = Image.open(path)
    print(f'Format: {img.format}, Mode: {img.mode}, Size: {img.size}')
    
    if img.mode == 'P':
        palette = img.getpalette()
        print(f'Palette length: {len(palette) if palette else 0}')
        if palette and len(palette) >= 3:
            print(f'Index 0 color: ({palette[0]}, {palette[1]}, {palette[2]})')
            
        colors = img.getcolors(256)
        print(f'Number of unique colors used: {len(colors) if colors else ">256"}')
        
        near_magentas = []
        for i in range(len(palette) // 3):
            r, g, b = palette[i*3], palette[i*3+1], palette[i*3+2]
            if r > 240 and g < 15 and b > 240:
                near_magentas.append((i, r, g, b))
                
        if near_magentas:
            print('Near-magenta colors found in palette:')
            for idx, r, g, b in near_magentas:
                print(f'  Index {idx}: ({r}, {g}, {b})')

analyze_image('Asset/graphics/stills/event/ui_msg_window.bmp')
analyze_image('Asset/graphics/stills/event/gba_event.bmp')
analyze_image('Asset/graphics/sprites/chara/chara/spr_ch_mayo_normal.bmp')
