#!/usr/bin/env python3
import os
import sys
from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
STILLS_DIR = os.path.join(PROJECT_ROOT, "Asset", "graphics", "stills")

def process_image(img):
    # Determine if padding is needed
    w, h = img.size
    pad_w = ((w + 255) // 256) * 256
    pad_h = ((h + 255) // 256) * 256
    
    needs_padding = (w != pad_w or h != pad_h)
    
    if needs_padding:
        new_img = Image.new('RGB', (pad_w, pad_h), (0, 0, 0)) # black background
        
        # Convert to RGBA for pasting if it has transparency
        if img.mode in ('RGBA', 'LA') or (img.mode == 'P' and 'transparency' in img.info):
            if img.mode == 'P':
                img = img.convert('RGBA')
            new_img.paste(img, (0, 0), mask=img.split()[3])
        else:
            if img.mode != 'RGB':
                img = img.convert('RGB')
            new_img.paste(img, (0, 0))
        img = new_img
        
    # Convert to 8-bit indexed (256 colors) if it isn't already
    if img.mode != 'P':
        img = img.convert('P', palette=Image.ADAPTIVE, colors=256)
        
    return img, needs_padding or img.mode != 'P'

def format_assets():
    if not os.path.exists(STILLS_DIR):
        print(f"Error: {STILLS_DIR} not found.")
        return
        
    print(f"Scanning {STILLS_DIR} for images to format...")
    processed_count = 0
    
    for root, dirs, files in os.walk(STILLS_DIR):
        for file in files:
            file_lower = file.lower()
            ext = os.path.splitext(file_lower)[1]
            
            if ext not in ['.png', '.bmp']:
                continue
                
            file_path = os.path.join(root, file)
            bmp_path = os.path.join(root, os.path.splitext(file_lower)[0] + '.bmp')
            
            # Rename if there are uppercase letters
            if file != file_lower:
                os.rename(file_path, os.path.join(root, file_lower))
                file_path = os.path.join(root, file_lower)
                print(f"Renamed: {file} -> {file_lower}")
                
            try:
                img = Image.open(file_path)
                processed_img, changed = process_image(img)
                
                if ext == '.png':
                    # Save as BMP, delete PNG
                    processed_img.save(bmp_path, 'BMP')
                    img.close()
                    os.remove(file_path)
                    print(f"Converted & Padded: {file} -> {os.path.basename(bmp_path)}")
                    processed_count += 1
                elif changed:
                    # Save to temp file then replace
                    temp_path = file_path + ".tmp"
                    processed_img.save(temp_path, 'BMP')
                    img.close()
                    os.replace(temp_path, bmp_path)
                    print(f"Padded & Indexed: {file}")
                    processed_count += 1
                else:
                    img.close()
            except Exception as e:
                print(f"Failed to process {file}: {e}")
                
    print(f"Done! Processed {processed_count} files.")

if __name__ == "__main__":
    format_assets()
