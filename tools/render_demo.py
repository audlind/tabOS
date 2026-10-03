import os
import sys

# 16-fargers ANSI palett (32-bit ARGB / BGRA for Allwinner fb0)
ANSI_PALETTE = [
    (0, 0, 0),        # 0: Svart
    (170, 0, 0),      # 1: Rød
    (0, 170, 0),      # 2: Grønn
    (170, 85, 0),     # 3: Brun
    (0, 0, 170),      # 4: Blå
    (170, 0, 170),    # 5: Magenta
    (0, 170, 170),    # 6: Cyan
    (170, 170, 170),  # 7: Lys grå
    (85, 85, 85),     # 8: Mørk grå
    (255, 85, 85),    # 9: Lys rød
    (85, 255, 85),    # 10: Lys grønn
    (255, 255, 85),   # 11: Gul
    (85, 85, 255),    # 12: Lys blå
    (255, 85, 255),   # 13: Lys magenta
    (85, 255, 255),   # 14: Lys cyan
    (255, 255, 255)   # 15: Hvit
]

def load_font():
    font_path = os.path.join(os.path.dirname(__file__), '..', 'include', 'font_cp437_8x16.h')
    font = []
    with open(font_path, 'r', encoding='utf-8') as f:
        text = f.read()
    import re
    hexes = re.findall(r'0x([0-9a-fA-F]{2})', text)
    bytes_data = [int(h, 16) for h in hexes[:4096]]
    for ch in range(256):
        font.append(bytes_data[ch*16:(ch+1)*16])
    return font

class DisplayEngine:
    def __init__(self, font, orientation='landscape'):
        self.font = font
        self.set_orientation(orientation)

    def set_orientation(self, orientation):
        self.orientation = orientation
        if orientation == 'portrait':
            self.cols = 60
            self.rows = 50
        else:
            self.cols = 100
            self.rows = 30
        self.grid = [{'ch': ord(' '), 'fg': 7, 'bg': 0} for _ in range(self.cols * self.rows)]

    def put_cell(self, col, row, ch, fg=7, bg=0):
        if 0 <= col < self.cols and 0 <= row < self.rows:
            self.grid[row * self.cols + col] = {
                'ch': ord(ch) if isinstance(ch, str) else ch,
                'fg': fg,
                'bg': bg
            }

    def draw_string(self, col, row, s, fg=7, bg=0):
        c = col
        r = row
        for ch in s:
            if ch == '\n':
                r += 1
                c = col
                continue
            self.put_cell(c, r, ch, fg, bg)
            c += 1

    def draw_box(self, col, row, w, h, title="", fg=7, bg=0, border_fg=14):
        c2 = col + w - 1
        r2 = row + h - 1
        # Hjørner
        self.put_cell(col, row, 0xDA, border_fg, bg) # ┌
        self.put_cell(c2,  row, 0xBF, border_fg, bg) # ┐
        self.put_cell(col, r2,  0xC0, border_fg, bg) # └
        self.put_cell(c2,  r2,  0xD9, border_fg, bg) # ┘
        for c in range(col + 1, c2):
            self.put_cell(c, row, 0xC4, border_fg, bg) # ─
            self.put_cell(c, r2,  0xC4, border_fg, bg)
        for r in range(row + 1, r2):
            self.put_cell(col, r, 0xB3, border_fg, bg) # │
            self.put_cell(c2,  r, 0xB3, border_fg, bg)
            for c in range(col + 1, c2):
                self.put_cell(c, r, ' ', fg, bg)
        if title:
            tcol = col + (w - len(title) - 2) // 2
            self.put_cell(tcol - 1, row, ' ', border_fg, bg)
            self.draw_string(tcol, row, title, 15, bg)
            self.put_cell(tcol + len(title), row, ' ', border_fg, bg)

    def draw_button(self, col, row, w, label, fg=15, bg=4, pressed=False):
        final_fg = bg if pressed else fg
        final_bg = fg if pressed else bg
        self.put_cell(col, row, '[', 11, final_bg)
        c = col + 1
        pad = (w - 2 - len(label)) // 2
        for _ in range(pad):
            self.put_cell(c, row, ' ', final_fg, final_bg)
            c += 1
        for ch in label:
            self.put_cell(c, row, ch, final_fg, final_bg)
            c += 1
        while c < col + w - 1:
            self.put_cell(c, row, ' ', final_fg, final_bg)
            c += 1
        self.put_cell(col + w - 1, row, ']', 11, final_bg)

    def render_to_framebuffer(self):
        # 800 x 480 32-bit ARGB bytes
        fb = bytearray(800 * 480 * 4)

        if self.orientation == 'landscape':
            for r in range(self.rows):
                for c in range(self.cols):
                    cell = self.grid[r * self.cols + c]
                    glyph_bits = self.font[cell['ch']]
                    fg_rgb = ANSI_PALETTE[cell['fg']]
                    bg_rgb = ANSI_PALETTE[cell['bg']]

                    base_x = c * 8
                    base_y = r * 16

                    for py in range(16):
                        row_val = glyph_bits[py]
                        y = base_y + py
                        row_offset = y * 800 * 4
                        for px in range(8):
                            x = base_x + px
                            color = fg_rgb if (row_val & (0x80 >> px)) else bg_rgb
                            idx = row_offset + x * 4
                            fb[idx]     = color[2] # B
                            fb[idx + 1] = color[1] # G
                            fb[idx + 2] = color[0] # R
                            fb[idx + 3] = 0xFF     # A
        else: # portrait
            # 60 x 50 rutenett -> 480 x 800 virtuelt -> rotert 90 grader til 800 x 480
            for r in range(self.rows):
                for c in range(self.cols):
                    cell = self.grid[r * self.cols + c]
                    glyph_bits = self.font[cell['ch']]
                    fg_rgb = ANSI_PALETTE[cell['fg']]
                    bg_rgb = ANSI_PALETTE[cell['bg']]

                    base_xv = c * 8
                    base_yv = r * 16

                    for py in range(16):
                        row_val = glyph_bits[py]
                        yv = base_yv + py
                        x_phys = 799 - yv

                        for px in range(8):
                            xv = base_xv + px
                            y_phys = xv

                            color = fg_rgb if (row_val & (0x80 >> px)) else bg_rgb
                            idx = (y_phys * 800 + x_phys) * 4
                            fb[idx]     = color[2] # B
                            fb[idx + 1] = color[1] # G
                            fb[idx + 2] = color[0] # R
                            fb[idx + 3] = 0xFF     # A

        return bytes(fb)

def create_landscape_demo():
    font = load_font()
    disp = DisplayEngine(font, 'landscape')

    # Topplinje / Header
    disp.draw_box(0, 0, 100, 3, "", 15, 4, 11)
    disp.draw_string(2, 1, "tabOS BBS TERMINAL v0.1", 15, 4)
    disp.draw_string(40, 1, "[ NODE 1 ONLINE ]", 10, 4)
    disp.draw_string(68, 1, "TID: 17:00:24  BAT: 85%", 14, 4)

    # ASCII-logo
    banner = [
        " _       _     ___  ____  ",
        "| |_ ___| |__ / _ \\/ ___| ",
        "| __/ _ \\ '_ \\ | | \\___ \\ ",
        "| ||  __/ |_) | |_| |___) |",
        " \\__\\___|_.__/ \\___/|____/ "
    ]
    for i, line in enumerate(banner):
        disp.draw_string(36, 4 + i, line, 11, 0) # Lys gul

    disp.draw_string(27, 10, "=== CYBERDECK 7\" ALLWINNER A13 EDITION ===", 13, 0)

    # Hovedmeny - 3 kolonner
    # Kolonne 1: Online BBS Doors
    disp.draw_box(4, 12, 28, 12, "BBS ONLINE PORTER", 15, 0, 14)
    disp.draw_button(6, 14, 24, " [1] TELEHACK BBS ", 15, 1, False)
    disp.draw_button(6, 17, 24, " [2] VERTRAUEN BBS", 15, 1, False)
    disp.draw_button(6, 20, 24, " [3] TITANTIC BBS ", 15, 1, False)

    # Kolonne 2: Innebygde Verktøy
    disp.draw_box(36, 12, 28, 12, "LOKALE APPER", 15, 0, 10)
    disp.draw_button(38, 14, 24, " [4] RETRO SNAKE  ", 15, 2, False)
    disp.draw_button(38, 17, 24, " [5] NOTISBLOKK   ", 15, 2, False)
    disp.draw_button(38, 20, 24, " [6] SYSTEMSTATUS ", 15, 2, False)

    # Kolonne 3: System & Innstillinger
    disp.draw_box(68, 12, 28, 12, "KONTROLLPANEL", 15, 0, 13)
    disp.draw_button(70, 14, 24, " [7] ROTER SKJERM ", 15, 5, False)
    disp.draw_button(70, 17, 24, " [8] ANSI FARGETEST", 15, 5, False)
    disp.draw_button(70, 20, 24, " [X] LOGG UT / AV ", 15, 5, False)

    # Bunnlinje / Touch-meny
    disp.draw_box(0, 26, 100, 4, "", 15, 8, 7)
    disp.draw_string(2, 27, "TOUCH-BAR: [ HJEM ]  [ TELNET ]  [ TASTATUR ]  [ ROTER ]  [ LYSSKJERM ]", 15, 8)
    disp.draw_string(2, 28, "STATUS   : Allwinner sun5i | 512MB DDR3 | 800x480 TFT | Zet6221 Touch OK", 14, 8)

    return disp.render_to_framebuffer()

def create_portrait_demo():
    font = load_font()
    disp = DisplayEngine(font, 'portrait')

    # Topplinje (60 kolonner x 50 rader)
    disp.draw_box(0, 0, 60, 3, "", 15, 4, 11)
    disp.draw_string(2, 1, "tabOS PORTRETT MODUS", 15, 4)
    disp.draw_string(36, 1, "[ 60x50 TEGN ]", 14, 4)

    # ASCII-logo
    banner = [
        " _       _     ___  ____  ",
        "| |_ ___| |__ / _ \\/ ___| ",
        "| __/ _ \\ '_ \\ | | \\___ \\ ",
        "| ||  __/ |_) | |_| |___) |",
        " \\__\\___|_.__/ \\___/|____/ "
    ]
    for i, line in enumerate(banner):
        disp.draw_string(17, 4 + i, line, 11, 0)

    disp.draw_string(10, 10, "=== CYBERDECK PORTABLE BBS ===", 13, 0)

    # Meny-boks (Vertikal layout)
    disp.draw_box(2, 12, 56, 16, "HOVEDMENY", 15, 0, 14)
    disp.draw_button(5, 14, 50, "  [1] TELEHACK ONLINE BBS  ", 15, 1, False)
    disp.draw_button(5, 17, 50, "  [2] VERTRAUEN SYNCHRONET ", 15, 1, False)
    disp.draw_button(5, 20, 50, "  [3] RETRO SNAKE SPILL    ", 15, 2, False)
    disp.draw_button(5, 23, 50, "  [4] NOTISBLOKK & EDITOR  ", 15, 2, False)

    # Status / Terminal Preview
    disp.draw_box(2, 29, 56, 6, "TERMINAL STATUS", 15, 0, 10)
    disp.draw_string(4, 31, "> System initialisert: 800x480 -> 480x800", 10, 0)
    disp.draw_string(4, 32, "> Roterer 90 grader med klokken", 14, 0)
    disp.draw_string(4, 33, "> Zet6221 I2C touch-driver aktiv", 15, 0)

    # STORT TOUCH-TASTATUR NEDERST (Rader 36 til 49)
    disp.draw_box(0, 36, 60, 14, "TOUCH TASTATUR", 15, 0, 7)
    disp.draw_string(3, 38, "[1] [2] [3] [4] [5] [6] [7] [8] [9] [0] [<-]", 15, 8)
    disp.draw_string(3, 40, " [Q] [W] [E] [R] [T] [Y] [U] [I] [O] [P] ", 14, 8)
    disp.draw_string(4, 42, " [A] [S] [D] [F] [G] [H] [J] [K] [L] [ENTER]", 10, 8)
    disp.draw_string(6, 44, "[Z] [X] [C] [V] [B] [N] [M] [ , ] [ . ]", 11, 8)
    disp.draw_string(4, 47, "[  MELLOMROM  ]   [ ROTER ]   [ AVSLUTT ]", 15, 1)

    return disp.render_to_framebuffer()

if __name__ == '__main__':
    fb_land = create_landscape_demo()
    with open('tabos_demo_landscape.bin', 'wb') as f:
        f.write(fb_land)
    print(f"Generated tabos_demo_landscape.bin ({len(fb_land)} bytes)")

    fb_port = create_portrait_demo()
    with open('tabos_demo_portrait.bin', 'wb') as f:
        f.write(fb_port)
    print(f"Generated tabos_demo_portrait.bin ({len(fb_port)} bytes)")

