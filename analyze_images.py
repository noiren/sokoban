import os
import json
from PIL import Image

results = []
root_dir = 'graphics/temp_uploads'
for root, dirs, files in os.walk(root_dir):
    for f in files:
        if f.lower().endswith('.png'):
            path = os.path.join(root, f)
            try:
                with Image.open(path) as img:
                    width, height = img.size
                    rel_dir = os.path.basename(root)
                    if rel_dir == 'temp_uploads': rel_dir = 'ルート'
                    
                    # すべての画像が240x160なので、用途はファイル名などから推定
                    img_type = '背景 / 背景用モックアップ'
                    
                    results.append({
                        'dir': rel_dir,
                        'file': f,
                        'width': width,
                        'height': height,
                        'type': img_type
                    })
            except Exception as e:
                pass

# Sort results by directory
results.sort(key=lambda x: x['dir'])

# Markdown生成
md_content = []
md_content.append("# 画像素材リスト (graphics/temp_uploads)")
md_content.append("")
md_content.append("| ディレクトリ | ファイル名 | サイズ | 推定用途 |")
md_content.append("|---|---|---|---|")
for r in results:
    md_content.append(f"| {r['dir']} | {r['file']} | {r['width']}x{r['height']} | {r['type']} |")

# ファイル書き出し
with open("image_analysis_report.md", "w", encoding="utf-8") as f:
    f.write("\n".join(md_content))
