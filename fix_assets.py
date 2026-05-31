import os
import glob
from PIL import Image

def fix_character_fringes(directory):
    print(f"Fixing character fringes in {directory}")
    for filepath in glob.glob(os.path.join(directory, "*.bmp")):
        filepath = os.path.normpath(os.path.abspath(filepath))
        img = Image.open(filepath)
        if img.mode != 'P':
            continue
            
        # Get palette
        palette = img.getpalette()
        if not palette:
            continue
            
        # Find index 0
        idx0_color = (palette[0], palette[1], palette[2])
        
        # Find near-magenta palette indices
        near_magenta_indices = []
        for i in range(len(palette) // 3):
            r, g, b = palette[i*3], palette[i*3+1], palette[i*3+2]
            if r > 240 and g < 15 and b > 240 and i != 0:
                near_magenta_indices.append(i)
                
        if near_magenta_indices:
            print(f"  {os.path.basename(filepath)}: Found near-magenta at indices {near_magenta_indices}")
            
            # Replace near-magenta pixels with index 0
            img_data = list(img.getdata())
            new_data = [0 if p in near_magenta_indices else p for p in img_data]
            
            img.putdata(new_data)
            temp_path = filepath + '.tmp'
            img.save(temp_path, format='BMP')
            img.close()
            os.replace(temp_path, filepath)
            print(f"    -> Fixed and saved.")
        else:
            img.close()
            
def reduce_ui_window_colors(filepath):
    print(f"Reducing colors for {os.path.basename(filepath)}")
    img = Image.open(filepath)
    if img.mode != 'P':
        img = img.convert('RGB')
    else:
        img = img.convert('RGB')
        
    # First, make sure magenta is exactly magenta
    img_data = list(img.getdata())
    new_data = []
    for r, g, b in img_data:
        if r > 240 and g < 15 and b > 240:
            new_data.append((255, 0, 255))
        else:
            new_data.append((r, g, b))
            
    img.putdata(new_data)
    
    # Quantize to 16 colors (15 colors + 1 magenta)
    # Using Image.Quantize.FASTOCTREE or libimagequant
    quantized = img.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
    
    # Ensure index 0 is magenta
    palette = quantized.getpalette()
    magenta_idx = -1
    for i in range(16):
        if palette[i*3] == 255 and palette[i*3+1] == 0 and palette[i*3+2] == 255:
            magenta_idx = i
            break
            
    if magenta_idx != -1 and magenta_idx != 0:
        # Swap index 0 and magenta_idx
        # Swap palette
        r0, g0, b0 = palette[0], palette[1], palette[2]
        rm, gm, bm = palette[magenta_idx*3], palette[magenta_idx*3+1], palette[magenta_idx*3+2]
        
        palette[0], palette[1], palette[2] = rm, gm, bm
        palette[magenta_idx*3], palette[magenta_idx*3+1], palette[magenta_idx*3+2] = r0, g0, b0
        
        quantized.putpalette(palette)
        
        # Swap pixels
        q_data = list(quantized.getdata())
        new_q_data = []
        for p in q_data:
            if p == 0:
                new_q_data.append(magenta_idx)
            elif p == magenta_idx:
                new_q_data.append(0)
            else:
                new_q_data.append(p)
        quantized.putdata(new_q_data)
        
    temp_path = filepath + '.tmp'
    quantized.save(temp_path, format='BMP')
    img.close()
    os.replace(temp_path, filepath)
    print("  -> Saved as 16-color image.")

# Fix character sprites
fix_character_fringes('Asset/graphics/sprites/chara/chara')

# Fix UI window
ui_path = 'Asset/graphics/stills/event/ui_msg_window.bmp'
reduce_ui_window_colors(ui_path)
