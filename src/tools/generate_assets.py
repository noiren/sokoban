#!/usr/bin/env python3
import json
import os
import struct
import re

import sys

if getattr(sys, 'frozen', False):
    SCRIPT_DIR = os.path.dirname(sys.executable)
else:
    SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)

ASSETS_LIST_PATH = os.path.join(SCRIPT_DIR, "assets_list.json")
MANIFEST_PATH = os.path.join(SCRIPT_DIR, "image_manifest.json")
MAKEFILE_PATH = os.path.join(PROJECT_ROOT, "Makefile")

# Create a valid indexed BMP (for sprites)
def create_dummy_bmp(filename, width, height):
    if os.path.exists(filename):
        return  # Do not overwrite if file already exists
    
    pixels = [1] * (width * height)
    # Palette: Transp = 0, Debug Color = 1
    palette = b'\x00\x00\x00\x00' + b'\x80\x80\x80\x00'
    palette_data = palette + b'\x00\x00\x00\x00' * 254

    row_bytes = width
    padding = (4 - (row_bytes % 4)) % 4
    pixel_data = b''.join([bytes(pixels[y*width:(y+1)*width]) + b'\x00'*padding for y in range(height-1, -1, -1)])
    
    header = struct.pack('<2sIHHI', b'BM', 54 + 1024 + len(pixel_data), 0, 0, 54 + 1024)
    info = struct.pack('<IiiHHIIIIII', 40, width, height, 1, 8, 0, len(pixel_data), 2835, 2835, 256, 256)
    
    with open(filename, 'wb') as f:
        f.write(header + info + palette_data + pixel_data)

def generate_butano_json(filename):
    pass

def main():
    if not os.path.exists(ASSETS_LIST_PATH):
        print(f"Error: {ASSETS_LIST_PATH} not found.")
        print("Please create it first.")
        return

    with open(ASSETS_LIST_PATH, 'r', encoding='utf-8') as f:
        config = json.load(f)
    
    manifest_data = {}
    directories = set(["graphics"]) # Base graphics folder
    
    for set_name, group_info in config.items():
        base_path = group_info.get("path", "graphics") # e.g. graphics/ui/menu
        size = group_info.get("size", [32, 32])
        items = group_info.get("items", [])
        
        # Ensure path starts with graphics
        if not base_path.startswith("graphics"):
            base_path = "graphics/" + base_path
            
        directories.add(base_path)
        
        # Make directories
        os.makedirs(os.path.join(PROJECT_ROOT, base_path), exist_ok=True)
        
        manifest_data[set_name] = {}
        
        for idx, item_name in enumerate(items):
            manifest_data[set_name][str(idx)] = f"{base_path}/{item_name}".replace("graphics/", "", 1)
            
            bmp_path = os.path.join(PROJECT_ROOT, base_path, f"{item_name}.bmp")
            json_path = os.path.join(PROJECT_ROOT, base_path, f"{item_name}.json")
            
            create_dummy_bmp(bmp_path, size[0], size[1])
            generate_butano_json(json_path)

    # 1. Update image_manifest.json
    with open(MANIFEST_PATH, 'w', encoding='utf-8') as f:
        json.dump(manifest_data, f, indent=2)
    print(f"Generated {MANIFEST_PATH}")

    # 2. Update Makefile is now handled by prebuild.py

if __name__ == '__main__':
    main()
