import png
import numpy as np
from PIL import Image
import sys

def floyd_steinberg_dither(image_array, levels=16):
    """Apply Floyd–Steinberg dithering and quantize to grayscale levels."""
    height, width = image_array.shape
    for y in range(height):
        for x in range(width):
            old = image_array[y, x]
            new_val = round(old * (levels - 1) / 255)
            new_pixel = new_val * (255 // (levels - 1))
            error = old - new_pixel
            image_array[y, x] = new_pixel

            if x + 1 < width:
                image_array[y, x + 1] += error * 7 / 16
            if y + 1 < height:
                if x > 0:
                    image_array[y + 1, x - 1] += error * 3 / 16
                image_array[y + 1, x] += error * 5 / 16
                if x + 1 < width:
                    image_array[y + 1, x + 1] += error * 1 / 16
    return np.clip(image_array, 0, 255).astype(np.uint8)

def save_4bit_grayscale_png(pil_image, output_path):
    gray = pil_image.convert('L')
    arr = np.array(gray, dtype=np.float32)
    dithered = floyd_steinberg_dither(arr)

    # Convert to 4-bit values (0–15)
    quantized = np.round(dithered / 255 * 15).astype(np.uint8)

    with open(output_path, 'wb') as f:
        writer = png.Writer(
            width=quantized.shape[1],
            height=quantized.shape[0],
            bitdepth=4,
            greyscale=True,
        )
        writer.write(f, quantized.tolist())

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python dither_to_4bit_grayscale_png.py input.png output.png")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    image = Image.open(input_path)
    save_4bit_grayscale_png(image, output_path)
    print(f"✅ Saved: {output_path}")