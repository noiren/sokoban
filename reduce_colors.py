import os
from PIL import Image

def reduce_colors(input_path, max_colors=240):
    img = Image.open(input_path).convert("RGBA")
    
    # Create a mask of which pixels are magenta (transparent)
    pixels = img.load()
    width, height = img.size
    
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            # Assuming pure magenta is transparent
            if r == 255 and g == 0 and b == 255:
                pixels[x, y] = (255, 0, 255, 0)
    
    # Quantize using PIL's built-in fast octree or median cut, keeping alpha
    # But wait, BMP doesn't support alpha. 
    # PIL's quantize can be tricky with exact palette forcing.
    # We can create a base image with the desired palette
    # Actually, PIL's quantize method can take an image with an alpha channel
    img_q = img.quantize(colors=max_colors-1)
    
    # Get the quantized palette
    q_palette = img_q.getpalette()
    
    # We need to ensure index 0 is magenta.
    # Create a new palette: Magenta + q_palette
    new_palette = [255, 0, 255] + q_palette[:(max_colors-1)*3]
    
    # Pad to 256 colors
    new_palette += [0, 0, 0] * (256 - len(new_palette) // 3)
    
    # Create the final image
    # We have to remap the quantized image indices by +1
    final_img = Image.new("P", img.size)
    final_img.putpalette(new_palette)
    
    # Map old indices to new (+1)
    q_data = img_q.getdata()
    
    # But wait! The transparent pixels in img_q might have been assigned some index.
    # Let's map pixels manually using Euclidean distance or just use the alpha channel we had.
    # Wait, PIL quantize usually handles transparency by putting it at one of the indices.
    # It's safer to just iterate and match manually, but it's slow.
    # Let's do a simple mapping.
    # First, let's just convert img to RGB (replacing transparent with magenta) and use adaptive quantization.
    pass

# Better approach for GBA BMP:
def simple_reduce(input_path, max_colors=240):
    img = Image.open(input_path).convert("RGB")
    pixels = list(img.getdata())
    
    # Identify unique colors
    unique_colors = list(set(pixels))
    
    # If already <= max_colors, just fix index 0
    # Otherwise, we need to quantize.
    q_img = img.quantize(colors=max_colors-1, method=2)
    q_pal = q_img.getpalette()
    
    new_pal = [255, 0, 255]
    for i in range(max_colors - 1):
        if i*3+2 < len(q_pal):
            new_pal.extend([q_pal[i*3], q_pal[i*3+1], q_pal[i*3+2]])
            
    new_pal += [0,0,0] * (256 - len(new_pal)//3)
    
    out_img = Image.new("P", img.size)
    out_img.putpalette(new_pal)
    
    # Now map pixels. If original pixel was magenta, index is 0.
    # Else, find nearest color in new_pal[3:]
    
    # PIL's im.quantize() already mapped the pixels, so we just shift indices by 1
    # EXCEPT for pixels that were originally magenta.
    q_data = list(q_img.getdata())
    orig_data = list(img.getdata())
    
    new_data = []
    for i in range(len(orig_data)):
        if orig_data[i] == (255, 0, 255):
            new_data.append(0)
        else:
            new_data.append(q_data[i] + 1)
            
    out_img.putdata(new_data)
    out_img.save(input_path)
    print(f"Reduced colors for {input_path}")

simple_reduce("Asset/graphics/stills/soukoban_gamentest/still_sokoban_main.bmp")
