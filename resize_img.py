from PIL import Image
import sys

TARGET_WIDTH = 1872 * 2 + 100    # 3794
TARGET_HEIGHT = 1404 * 4 + 300   # 7788

def resize_and_center_crop(image: Image.Image, target_width: int, target_height: int) -> Image.Image:
    src_width, src_height = image.size
    src_ratio = src_width / src_height
    target_ratio = target_width / target_height

    # Determine scaling factor to preserve aspect ratio
    if src_ratio > target_ratio:
        # Image is wider than target: fit to height
        scale = target_height / src_height
    else:
        # Image is taller or equal: fit to width
        scale = target_width / src_width

    new_width = int(round(src_width * scale))
    new_height = int(round(src_height * scale))
    resized = image.resize((new_width, new_height), Image.LANCZOS)

    # Center crop to target size
    left = (new_width - target_width) // 2
    top = (new_height - target_height) // 2
    right = left + target_width
    bottom = top + target_height

    cropped = resized.crop((left, top, right, bottom))
    return cropped

def process(input_path: str, output_path: str):
    image = Image.open(input_path)
    processed = resize_and_center_crop(image, TARGET_WIDTH, TARGET_HEIGHT)
    processed.save(output_path)
    print(f"✅ Saved resized & center-cropped image to: {output_path} ({TARGET_WIDTH}x{TARGET_HEIGHT})")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python resize_center_crop_exact.py input.jpg output.jpg")
        sys.exit(1)

    process(sys.argv[1], sys.argv[2])