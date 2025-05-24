from PIL import Image
import sys
import os

# Tile dimensions
TILE_WIDTH = 1872
TILE_HEIGHT = 1404

# Step between tiles
STEP_X = 1972  # horizontal step
STEP_Y = 1504  # vertical step

def tile_image(input_path: str, output_dir: str):
    image = Image.open(input_path)
    width, height = image.size

    tiles_x = (width - TILE_WIDTH) // STEP_X + 1
    tiles_y = (height - TILE_HEIGHT) // STEP_Y + 1

    os.makedirs(output_dir, exist_ok=True)

    for row in range(tiles_y):
        for col in range(tiles_x):
            left = col * STEP_X
            top = row * STEP_Y
            right = left + TILE_WIDTH
            bottom = top + TILE_HEIGHT

            if right > width or bottom > height:
                continue  # skip tiles that exceed bounds

            tile = image.crop((left, top, right, bottom))
            tile_path = os.path.join(output_dir, f"{col}-{row}.png")
            tile.save(tile_path)

    print(f"✅ Saved {tiles_x * tiles_y} tiles to '{output_dir}/'")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python tile_image_step_swapped.py input.png output_dir")
        sys.exit(1)

    tile_image(sys.argv[1], sys.argv[2])