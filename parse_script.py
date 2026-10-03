import struct

with open('script.bin', 'rb') as f:
    data = f.read()

sections_count, filesize, ver0, ver1 = struct.unpack('<IIII', data[0:16])

offset = 16
sections = []
for i in range(sections_count):
    name_raw = data[offset:offset+32]
    name = name_raw.split(b'\x00')[0].decode('latin1')
    length, sec_offset = struct.unpack('<ii', data[offset+32:offset+40])
    sections.append((name, length, sec_offset))
    offset += 40

with open('sys_config.fex', 'w', encoding='utf-8') as out_f:
    out_f.write(f"; Decompiled Allwinner script.bin for Q88B Tablet\n; Total Sections: {sections_count}\n\n")
    
    for name, length, sec_offset in sections:
        out_f.write(f"[{name}]\n")
        entry_offset = sec_offset * 4
        for e in range(length):
            entry_name_raw = data[entry_offset:entry_offset+32]
            entry_name = entry_name_raw.split(b'\x00')[0].decode('latin1')
            val_offset, pattern = struct.unpack('<II', data[entry_offset+32:entry_offset+40])
            type_hi = pattern >> 16
            words_lo = pattern & 0xffff
            
            val_pos = val_offset * 4
            line = ""
            
            if type_hi == 1: # Integer
                val_int = struct.unpack('<i', data[val_pos:val_pos+4])[0]
                line = f"{entry_name} = {val_int}"
            elif type_hi == 2: # String
                s = data[val_pos:val_pos+words_lo*4].split(b'\x00')[0].decode('latin1', errors='ignore')
                line = f"{entry_name} = \"{s}\""
            elif type_hi == 4: # GPIO (6 words)
                port, port_num, mul_sel, pull, drv_level, data_val = struct.unpack('<iiiiii', data[val_pos:val_pos+24])
                port_letter = chr(ord('A') + port - 1) if (1 <= port <= 26) else 'power'
                line = f"{entry_name} = port:P{port_letter}{port_num}<{mul_sel}><{pull}><{drv_level}><{data_val}>"
            elif type_hi == 5: # Null / empty
                line = f"{entry_name} ="
            else:
                line = f"; Unknown type {type_hi} for {entry_name}"
                
            if line:
                out_f.write(f"{line}\n")
            entry_offset += 40
        out_f.write("\n")

print("Successfully written complete decompiled sys_config.fex!")
