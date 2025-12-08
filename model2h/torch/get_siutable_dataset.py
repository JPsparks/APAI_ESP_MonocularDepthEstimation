import os
import glob
import numpy as np
from PIL import Image

import sys
import shutil

# --- CONFIGURAZIONE ---
INPUT_DIR = "raw_dataset"  # Get dir where there are raw photos (256x256 o altro)
OUTPUT_DIR = "ready_dataset"   # Specify where you want save cleaned results (48x48)
TARGET_SIZE = 1000                   # Quante foto generare al massimo
MIN_PHOTOS = 50

def ensure_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)
        print(f"Create dir: {directory}")

def process_image(image_path, save_path):
    try:
        img = Image.open(image_path).convert("RGB")
        
        # Handle not homogeneous inputs
        img = img.resize((256, 256))
        arr = np.array(img)
        
        # Centered crop to 240x240
        arr_240 = arr[8:248, 8:248, :] 
        
        arr_48 = arr_240[2::5, 2::5, :]
        
        if arr_48.shape != (48, 48, 3):
            print(f"Skip {image_path}: errate shape after crop {arr_48.shape}")
            return False

        Image.fromarray(arr_48).save(save_path)
        return True
        
    except Exception as e:
        print(f"ERR on {image_path}: {e}")
        return False





# Cerca formati comuni
files = []
for ext in ['*.jpg', '*.jpeg', '*.png', '*.bmp']:
    files.extend(glob.glob(os.path.join(INPUT_DIR, ext)))

amount_files = len(files)
print(f"Find {amount_files} file{'s' if amount_files > 0 else ''} in {INPUT_DIR}")

if amount_files < MIN_PHOTOS:
    print(f"ERR: expected at least {MIN_PHOTOS} pictures!")
    sys.exit(1)


# Checks existence AND checks if the directory has any files/folders in it.
if os.path.isdir(OUTPUT_DIR) and os.listdir(OUTPUT_DIR):
    shutil.rmtree(OUTPUT_DIR)
    print(f"Deleted non-empty directory: {OUTPUT_DIR}")


ensure_dir(OUTPUT_DIR)


count = 0
for i, fpath in enumerate(files):
    if count >= TARGET_SIZE:
        break
        
    filename = f"calib_{count:04d}.png"
    out_path = os.path.join(OUTPUT_DIR, filename)
    
    if process_image(fpath, out_path):
        count += 1
        if count % 10 == 0:
            print(f"Processed {count} pictures...")

print(f"--- Fin. {count} pictures stored in '{OUTPUT_DIR}' ---")

