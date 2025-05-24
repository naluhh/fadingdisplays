import subprocess
import os
import sys
import requests
from concurrent.futures import ThreadPoolExecutor

# Constants
RESIZED_IMAGE = "resized.png"
TILES_DIR = "tiles"
BPP_DIR = "tiles_4bpp"

# Devices and tile coordinates
DEVICES = [
    ("192.168.1.92",  0, 0),
    ("192.168.1.88",  1, 0),
    ("192.168.1.159", 0, 1),
    ("192.168.1.66",  1, 1),
    ("192.168.1.189", 0, 2),
    ("192.168.1.124", 1, 2),
    ("192.168.1.195", 0, 3),
    ("192.168.1.113", 1, 3),
]

# CLI Argument
if len(sys.argv) != 2:
    print("Usage: python dispatch_images.py path/to/input.jpg")
    sys.exit(1)

INPUT_IMAGE = sys.argv[1]

os.makedirs(TILES_DIR, exist_ok=True)
os.makedirs(BPP_DIR, exist_ok=True)

# Resize
print("🔧 Resizing image...")
subprocess.run(["python", "resize_img.py", INPUT_IMAGE, RESIZED_IMAGE], check=True)

# Cut
print("✂️  Cutting image into tiles...")
subprocess.run(["python", "cut_img.py", RESIZED_IMAGE, TILES_DIR], check=True)

# Function: Convert tile to 4bpp
def convert_tile(x, y):
    input_tile = os.path.join(TILES_DIR, f"{x}-{y}.png")
    output_tile = os.path.join(BPP_DIR, f"{x}-{y}.png")
    subprocess.run(["python", "to_4bpp.py", input_tile, output_tile], check=True)
    return (x, y, output_tile)

# Function: Upload tile
def upload_tile(x, y, ip):
    tile_path = os.path.join(BPP_DIR, f"{x}-{y}.png")
    try:
        with open(tile_path, "rb") as f:
            res = requests.post(
                f"http://{ip}:8890/display",
                headers={"Content-Type": "image/png"},
                data=f
            )
        if res.status_code == 200:
            print(f"✅ Uploaded {x}-{y}.png to {ip}")
        else:
            print(f"❌ Failed to upload {x}-{y}.png to {ip}: HTTP {res.status_code}")
    except Exception as e:
        print(f"❌ Exception for {ip}: {e}")

# Parallel conversion
print("🎨 Converting to 4bpp in parallel...")
with ThreadPoolExecutor(max_workers=8) as executor:
    futures = [
        executor.submit(convert_tile, x, y)
        for _, x, y in DEVICES
    ]
    results = [f.result() for f in futures]

# Parallel upload
print("📡 Uploading images to devices in parallel...")
with ThreadPoolExecutor(max_workers=8) as executor:
    futures = [
        executor.submit(upload_tile, x, y, ip)
        for ip, x, y in DEVICES
    ]
    [f.result() for f in futures]