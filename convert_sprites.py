import sys
from PIL import Image
import struct
import os

def convert_to_c_array(input_path, output_path, var_name, width, height):
    try:
        img = Image.open(input_path)
        img = img.resize((width, height), Image.Resampling.LANCZOS)
        img = img.convert('RGB')
    except FileNotFoundError:
        print(f"Error: File {input_path} not found.")
        return

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
            r, g, b = pixel
            # Convert to RGB565
            r5 = (r >> 3) & 0x1F
            g6 = (g >> 2) & 0x3F
            b5 = (b >> 3) & 0x1F
            
            rgb565 = (r5 << 11) | (g6 << 5) | b5
            
            # Little endian
            low_byte = rgb565 & 0xFF
            high_byte = (rgb565 >> 8) & 0xFF
            
            f.write(f'0x{low_byte:02x}, 0x{high_byte:02x}, ')
            
            if (i + 1) % 16 == 0:
                f.write('\n')
        
        f.write('};\n\n')
        
        f.write(f'const lv_img_dsc_t {var_name} = {{\n')
        f.write('  .header.cf = LV_IMG_CF_TRUE_COLOR,\n')
        f.write('  .header.always_zero = 0,\n')
        f.write('  .header.reserved = 0,\n')
        f.write(f'  .header.w = {width},\n')
        f.write(f'  .header.h = {height},\n')
        f.write(f'  .data_size = {width * height * 2},\n')
        f.write(f'  .data = {var_name}_map,\n')
        f.write('};\n')

if __name__ == '__main__':
    # Convert standard sprite to 66x66
    convert_to_c_array('standard.png', 'src/standard.c', 'standard', 66, 66)
