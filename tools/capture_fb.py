"""
Reads 1536000 bytes from /dev/graphics/fb0 on tablet and saves as 32-bit BMP.
"""
import subprocess
import struct

def capture():
    subprocess.run(["adb", "-s", "20080411413fc082", "shell", "dd if=/dev/graphics/fb0 of=/data/local/tmp/cap.bin bs=1536000 count=1 2>/dev/null"], check=True)
    subprocess.run(["adb", "-s", "20080411413fc082", "pull", "/data/local/tmp/cap.bin", "captured_fb.bin"], check=True)
    
    with open("captured_fb.bin", "rb") as f:
        raw_pixels = f.read()
    
    width = 800
    height = 480
    file_size = 54 + len(raw_pixels)
    
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
    bmp_header += struct.pack('<I', len(raw_pixels))
    bmp_header += struct.pack('<i', 2835)
    bmp_header += struct.pack('<i', 2835)
    bmp_header += struct.pack('<I', 0)
    bmp_header += struct.pack('<I', 0)
    
    with open("captured_screen.bmp", "wb") as f:
        f.write(bmp_header + raw_pixels)
    
    print("Lagret captured_screen.bmp (800x480)!")

if __name__ == '__main__':
    capture()
