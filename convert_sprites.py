import sys
from PIL import Image
import struct
import os

def convert_to_c_array(input_path, output_path, var_name, width, height):
    try:
        img = Image.open(input_path)
    except FileNotFoundError:
        print(f"Error: File {input_path} not found.")
        return
    
    # print(f"Image mode: {img.mode}")
    # if img.mode in ('1', 'L', 'P'):
    #     print("Image is black and white or grayscale")
    # else:
    #     print("Image is color")

    img = img.resize((width, height), Image.Resampling.LANCZOS)
    # img = img.convert('RGB')
    # grayscale_img = img.convert("L") 
    img = img.convert('1', dither=Image.Dither.FLOYDSTEINBERG) # B/W

    with open(output_path, 'w') as f:
        f.write('#include <lvgl.h>\n\n')
        f.write('#ifndef LV_ATTRIBUTE_MEM_ALIGN\n')
        f.write('#define LV_ATTRIBUTE_MEM_ALIGN\n')
        f.write('#endif\n\n')

        macro_name = f"LV_ATTRIBUTE_IMG_{var_name.upper()}"
        f.write(f'#ifndef {macro_name}\n')
        f.write(f'#define {macro_name}\n')
        f.write('#endif\n\n')

        f.write(f'const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST {macro_name} uint8_t {var_name}_map[] = {{\n')

        pixels = list(img.getdata())
        for i, pixel in enumerate(pixels):
            # r, g, b = pixel

            f.write(f'0x{pixel:02x},')

            if (i + 1) % width == 0:
                f.write('\n')

        f.write('};\n\n')

        f.write(f'const lv_img_dsc_t {var_name} = {{\n')
        f.write(f'  .header.magic = LV_IMAGE_HEADER_MAGIC,\n')
        f.write(f'  .header.cf = LV_COLOR_FORMAT_L8,\n')
        f.write(f'  .header.w = {width},\n')
        f.write(f'  .header.h = {height},\n')
        f.write(f'  .data_size = {width * height},\n')
        f.write(f'  .data = {var_name}_map,\n')
        f.write('};\n')

if __name__ == '__main__':
    if len(sys.argv) < 4:
        name = sys.argv[1]
        print(f"Usage: {sys.argv[0]} input.png output.c c_image_name [width] [height]")
        exit(1)

    img_input = sys.argv[1]
    c_output = sys.argv[2]
    c_name = sys.argv[3]

    width = int(sys.argv[4]) if len(sys.argv) > 4 else 128
    height = int(sys.argv[5]) if len(sys.argv) > 5 else 128

    convert_to_c_array(img_input, c_output, c_name, width, height)
