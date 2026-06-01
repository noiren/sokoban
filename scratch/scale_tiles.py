import os
import io
from PIL import Image

tiles_dir = os.path.join("Asset", "graphics", "tiles")

def scale_tiles():
    if not os.path.exists(tiles_dir):
        print(f"Directory {tiles_dir} does not exist.")
        return
        
    for file in os.listdir(tiles_dir):
        if file.endswith(".bmp"):
            path = os.path.join(tiles_dir, file)
            temp_path = path + "_temp.bmp"
            scaled = False
            try:
                with open(path, "rb") as f:
                    img_data = f.read()
                
                with Image.open(io.BytesIO(img_data)) as img:
                    if img.size == (8, 8):
                        print(f"Scaling {file} from 8x8 to 16x16...")
                        new_img = img.resize((16, 16), Image.NEAREST)
                        # Now we can safely overwrite path because the file is not locked by PIL
                        new_img.save(path)
                    else:
                        print(f"Skipping {file}, already {img.size}")
            except Exception as e:
                print(f"Failed to process {file}: {e}")

if __name__ == "__main__":
    scale_tiles()
