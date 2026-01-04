
import sys
import struct

def read_varint(f):
    value = 0
    while True:
        byte_data = f.read(1)
        if not byte_data:
            raise ValueError("Unexpected EOF while reading varint")
        byte = byte_data[0]
        value = (value << 7) | (byte & 0x7F)
        if not (byte & 0x80):
            break
    return value

def convert_wbmp_to_c(input_path, output_path, var_name):
    with open(input_path, 'rb') as f:
        # Header
        type_byte = f.read(1)
        fixed_header = f.read(1)
        
        if type_byte != b'\x00' or fixed_header != b'\x00':
             print(f"Warning: Unexpected WBMP header: {type_byte} {fixed_header}, proceeding anyway")

        width = read_varint(f)
        height = read_varint(f)
        
        print(f"Converting {input_path} ({width}x{height}) to {output_path}")
        
        # Data is row-major, 1 bit per pixel.
        # Check stride? WBMP: "Rows are padded to byte boundary?" No, "There is no padding between rows." 
        # Actually, usually in WBMP, rows ARE byte aligned?
        # WAP WAE: "The data ... is organized in rows ... Each row is byte-aligned."
        # OK, I should check if I need to handle padding.
        
        stride = (width + 7) // 8
        
        with open(output_path, 'w') as out:
            out.write('#include <lvgl.h>\n\n')
            out.write('#ifndef LV_ATTRIBUTE_MEM_ALIGN\n')
            out.write('#define LV_ATTRIBUTE_MEM_ALIGN\n')
            out.write('#endif\n\n')

            macro_name = f"LV_ATTRIBUTE_IMG_{var_name.upper()}"
            out.write(f'#ifndef {macro_name}\n')
            out.write(f'#define {macro_name}\n')
            out.write('#endif\n\n')

            out.write(f'const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST {macro_name} uint8_t {var_name}_map[] = {{\n')
            
            # Read all remaining data
            # data = f.read()
            
            # Iterate rows
            for y in range(height):
                row_bytes = f.read(stride)
                if len(row_bytes) < stride:
                     print("Warning: unexpected EOF in image data")
                     break
                     
                for x in range(width):
                    byte_idx = x // 8
                    bit_idx = 7 - (x % 8)
                    byte_val = row_bytes[byte_idx]
                    bit_val = (byte_val >> bit_idx) & 1
                    
                    # Convert to L8: 0 -> 0x00 (Black), 1 -> 0xFF (White)
                    # WBMP: 0 is black, 1 is white.
                    pixel_val = 0xFF if bit_val else 0x00
                    
                    out.write(f'0x{pixel_val:02x},')
                out.write('\n')

            out.write('};\n\n')

            out.write(f'const lv_img_dsc_t {var_name} = {{\n')
            out.write(f'  .header.magic = LV_IMAGE_HEADER_MAGIC,\n')
            out.write(f'  .header.cf = LV_COLOR_FORMAT_L8,\n')
            out.write(f'  .header.w = {width},\n')
            out.write(f'  .header.h = {height},\n')
            out.write(f'  .data_size = {width * height},\n')
            out.write(f'  .data = {var_name}_map,\n')
            out.write('};\n')

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} input.wbmp output.c c_image_name")
        exit(1)
        
    convert_wbmp_to_c(sys.argv[1], sys.argv[2], sys.argv[3])
