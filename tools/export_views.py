"""
Exports BMP screenshots of all tabOS views:
- ANSI Color Test (Landscape & Portrait)
- Touch Test & Canvas (Landscape & Portrait)
- Enlarged Keyboard (Landscape & Portrait)
"""
import sys
import os
import struct
sys.path.insert(0, os.path.dirname(__file__))
from tabos_runtime import TabOSSystem

def save_bmp(data, filename, width=800, height=480):
    file_size = 54 + len(data)
    bmp_header = bytearray()
    bmp_header += b'BM'
    bmp_header += struct.pack('<I', file_size)
    bmp_header += struct.pack('<HH', 0, 0)
    bmp_header += struct.pack('<I', 54)
    bmp_header += struct.pack('<I', 40)
    bmp_header += struct.pack('<i', width)
    bmp_header += struct.pack('<i', -height) # top-down
    bmp_header += struct.pack('<H', 1)
    bmp_header += struct.pack('<H', 32)
    bmp_header += struct.pack('<I', 0)
    bmp_header += struct.pack('<I', len(data))
    bmp_header += struct.pack('<i', 2835)
    bmp_header += struct.pack('<i', 2835)
    bmp_header += struct.pack('<I', 0)
    bmp_header += struct.pack('<I', 0)
    with open(filename, "wb") as f:
        f.write(bmp_header + data)
    print(f"Lagret {filename}")

def export_all():
    os_sys = TabOSSystem()
    
    # 1. Color Test
    os_sys.current_view = 'colortest'
    os_sys.orientation = 'landscape'
    os_sys.disp.set_orientation('landscape')
    save_bmp(os_sys.render(), "view_colortest_landscape.bmp")

    os_sys.orientation = 'portrait'
    os_sys.disp.set_orientation('portrait')
    save_bmp(os_sys.render(), "view_colortest_portrait.bmp")

    # 2. Touch Test
    os_sys.current_view = 'touchtest'
    os_sys.draw_points[(20, 15)] = 10
    os_sys.draw_points[(21, 15)] = 10
    os_sys.draw_points[(22, 16)] = 14
    os_sys.draw_points[(23, 17)] = 11
    os_sys.orientation = 'landscape'
    os_sys.disp.set_orientation('landscape')
    save_bmp(os_sys.render(), "view_touchtest_landscape.bmp")

    os_sys.orientation = 'portrait'
    os_sys.disp.set_orientation('portrait')
    save_bmp(os_sys.render(), "view_touchtest_portrait.bmp")

    # 3. Keyboard
    os_sys.current_view = 'keyboard'
    os_sys.typed_text = "tabOS CYBERDECK 2026: OK!"
    os_sys.orientation = 'landscape'
    os_sys.disp.set_orientation('landscape')
    save_bmp(os_sys.render(), "view_keyboard_landscape.bmp")

    os_sys.orientation = 'portrait'
    os_sys.disp.set_orientation('portrait')
    save_bmp(os_sys.render(), "view_keyboard_portrait.bmp")

if __name__ == '__main__':
    export_all()
