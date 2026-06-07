#!/usr/bin/env python3
import os
import shutil
import json
import re
import sys

# Import ui_compiler and generate_fix_data
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.append(SCRIPT_DIR)
import ui_compiler
import generate_fix_data
import pack_tiles
import generate_sprite_anims
import generate_chara_portraits

PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
SRC_GRAPHICS_DIR = os.path.join(PROJECT_ROOT, "Asset", "graphics")
TMP_GRAPHICS_DIR = os.path.join(PROJECT_ROOT, "build", "asset_tmp")
MAKEFILE_PATH = os.path.join(PROJECT_ROOT, "Makefile")

def prepare_graphics():
    if os.path.exists(TMP_GRAPHICS_DIR):
        shutil.rmtree(TMP_GRAPHICS_DIR)
    
    os.makedirs(TMP_GRAPHICS_DIR, exist_ok=True)
    
    directories_with_bmp = set()

    # 1. Copy BMPs and generate JSONs
    for root, _, files in os.walk(SRC_GRAPHICS_DIR):
        for file in files:
            if file.endswith(".bmp"):
                rel_dir = os.path.relpath(root, SRC_GRAPHICS_DIR)
                parts = rel_dir.split(os.sep)
                if "tiles" in parts:
                    continue  # 作業用の個別タイルフォルダはButanoの直接コンパイルから除外する
                
                src_path = os.path.join(root, file)
                
                # build/asset_tmp/stills, build/asset_tmp/sprites etc
                dst_sub_dir = os.path.join(TMP_GRAPHICS_DIR, rel_dir)
                os.makedirs(dst_sub_dir, exist_ok=True)
                
                # We need the path relative to Makefile for GRAPHICS var
                # e.g. build/asset_tmp/sprites/chara
                rel_to_proj = os.path.relpath(dst_sub_dir, PROJECT_ROOT).replace("\\", "/")
                directories_with_bmp.add(rel_to_proj)

                dst_path = os.path.join(dst_sub_dir, file)
                shutil.copy2(src_path, dst_path)

                # Check if a custom JSON exists in the source dir
                src_json_path = src_path.replace(".bmp", ".json")
                json_path = dst_path.replace(".bmp", ".json")
                
                parts = rel_dir.split(os.sep)
                is_still = "stills" in parts

                if os.path.exists(src_json_path):
                    shutil.copy2(src_json_path, json_path)
                else:
                    # Generate JSON
                    data = {"type": "regular_bg"} if is_still else {"type": "sprite"}
                    
                    with open(json_path, "w", encoding="utf-8") as f:
                        json.dump(data, f, indent=2)

    # 2. Update Makefile
    if os.path.exists(MAKEFILE_PATH) and directories_with_bmp:
        with open(MAKEFILE_PATH, "r", encoding="utf-8") as f:
            content = f.read()
        
        dirs_str = " ".join(sorted(directories_with_bmp))
        new_line = f"GRAPHICS    \t:=  {dirs_str}"
        
        # Only write if it actually changed to prevent unnecessary rebuilds
        match = re.search(r'^GRAPHICS\s*:=\s*(.*)$', content, flags=re.MULTILINE)
        if match and match.group(1).strip() != dirs_str:
            content = re.sub(r'^GRAPHICS\s*:=\s*.*$', new_line, content, flags=re.MULTILINE)
            with open(MAKEFILE_PATH, "w", encoding="utf-8") as f:
                f.write(content)
            print(f"[prebuild] Updated Makefile GRAPHICS: {dirs_str}")

def main():
    print("[prebuild] Packing individual 8x8 tiles into texture atlas...")
    pack_tiles.pack()
    
    print("[prebuild] Preparing graphics assets...")
    prepare_graphics()
    
    print("[prebuild] Running UI Compiler (+ audio manifest codegen)...")
    ui_compiler.main()

    print("[prebuild] Running FixDataManager codegen...")
    generate_fix_data.main()

    print("[prebuild] Running SpriteAnim codegen...")
    generate_sprite_anims.main()

    print("[prebuild] Running Chara Portraits codegen...")
    generate_chara_portraits.main()

    print("[prebuild] Prebuild tasks complete.")

if __name__ == "__main__":
    main()
