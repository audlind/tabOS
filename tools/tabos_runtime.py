#!/usr/bin/env python3
"""
tabOS Interactive Live Runtime
Brobygger mellom vert og nettbrett (Allwinner A13).
Støtter:
- Hovedmeny (Landskap & Portrett)
- ANSI Fargetest & Grafikk-kapasitet (full palett, CP437 tegnsett, rammer, gradienter)
- Touch-test & Tegneflate (sanntids koordinater og finger-tegning)
- Forstørret Touch-tastatur (store knapper, levende skrivefelt, slett & mellomrom)
"""

import os
import sys
import time
import subprocess
import threading
import queue

ADB_DEVICE = "20080411413fc082"

# 16-fargers ANSI palett (ARGB 32-bit for Allwinner A13 fb0)
ANSI_PALETTE = [
    (0, 0, 0),        # 0: Svart
    (170, 0, 0),      # 1: Rød
    (0, 170, 0),      # 2: Grønn
    (170, 85, 0),     # 3: Brun / Mørk gul
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

COLOR_NAMES = [
    "Svart", "Rod", "Gronn", "Brun",
    "Bla", "Magenta", "Cyan", "Lys gra",
    "Mork gra", "Lys rod", "Lys gronn", "Gul",
    "Lys bla", "Lys magenta", "Lys cyan", "Hvit"
]

def load_font():
    font_path = os.path.join(os.path.dirname(__file__), '..', 'include', 'font_cp437_8x16.h')
    font = []
    with open(font_path, 'r', encoding='utf-8') as f:
        text = f.read()
    import re
    text_clean = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    hexes = re.findall(r'0x([0-9a-fA-F]{2})', text_clean)
    bytes_data = [int(h, 16) for h in hexes[:4096]]
    for ch in range(256):
        font.append(bytes_data[ch*16:(ch+1)*16])
    return font

CP437_UNICODE_MAP = {
    '┌': 0xDA, '┐': 0xBF, '└': 0xC0, '┘': 0xD9, '─': 0xC4, '│': 0xB3,
    '╔': 0xC9, '╗': 0xBB, '╚': 0xC8, '╝': 0xBC, '═': 0xCD, '║': 0xBA,
    '┬': 0xC2, '┴': 0xC1, '├': 0xC3, '┤': 0xB4, '┼': 0xC5,
    '╦': 0xCB, '╩': 0xCA, '╠': 0xCC, '╣': 0xB9, '╬': 0xCE,
    '░': 0xB0, '▒': 0xB1, '▓': 0xB2, '█': 0xDB,
    '☺': 0x01, '☻': 0x02, '♥': 0x03, '♦': 0x04, '♣': 0x05, '♠': 0x06,
    '•': 0x07, '◘': 0x08, '○': 0x09, '◙': 0x0A, '♂': 0x0B, '♀': 0x0C,
    '♪': 0x0D, '♫': 0x0E, '☼': 0x0F, '►': 0x10, '◄': 0x11, '↕': 0x12,
    '‼': 0x13, '¶': 0x14, '§': 0x15, '▬': 0x16, '↨': 0x17, '↑': 0x18,
    '↓': 0x19, '→': 0x1A, '←': 0x1B, '∟': 0x1C, '↔': 0x1D, '▲': 0x1E, '▼': 0x1F,
    '±': 0xF1, '≥': 0xF2, '≤': 0xF3, '÷': 0xF6, '≈': 0xF7, '°': 0xF8, '√': 0xFB, '²': 0xFD,
    'æ': 0x91, 'Æ': 0x92, 'å': 0x86, 'Å': 0x8F, 'ø': 0x9B, 'Ø': 0x9D
}

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
        self.clear()

    def clear(self, bg=0):
        self.grid = [{'ch': ord(' '), 'fg': 7, 'bg': bg} for _ in range(self.cols * self.rows)]

    def put_cell(self, col, row, ch, fg=7, bg=0):
        if 0 <= col < self.cols and 0 <= row < self.rows:
            if isinstance(ch, str):
                if ch in CP437_UNICODE_MAP:
                    code = CP437_UNICODE_MAP[ch]
                else:
                    code = ord(ch)
            else:
                code = ch
            code = code % 256
            self.grid[row * self.cols + col] = {
                'ch': code,
                'fg': fg % 16,
                'bg': bg % 16
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
            if tcol > col:
                self.put_cell(tcol - 1, row, ' ', border_fg, bg)
                self.draw_string(tcol, row, title, 15, bg)
                self.put_cell(tcol + len(title), row, ' ', border_fg, bg)

    def draw_double_box(self, col, row, w, h, title="", fg=7, bg=0, border_fg=14):
        c2 = col + w - 1
        r2 = row + h - 1
        self.put_cell(col, row, 0xC9, border_fg, bg) # ╔
        self.put_cell(c2,  row, 0xBB, border_fg, bg) # ╗
        self.put_cell(col, r2,  0xC8, border_fg, bg) # ╚
        self.put_cell(c2,  r2,  0xBC, border_fg, bg) # ╝
        for c in range(col + 1, c2):
            self.put_cell(c, row, 0xCD, border_fg, bg) # ═
            self.put_cell(c, r2,  0xCD, border_fg, bg)
        for r in range(row + 1, r2):
            self.put_cell(col, r, 0xBA, border_fg, bg) # ║
            self.put_cell(c2,  r, 0xBA, border_fg, bg)
            for c in range(col + 1, c2):
                self.put_cell(c, r, ' ', fg, bg)
        if title:
            tcol = col + (w - len(title) - 2) // 2
            if tcol > col:
                self.put_cell(tcol - 1, row, ' ', border_fg, bg)
                self.draw_string(tcol, row, title, 15, bg)
                self.put_cell(tcol + len(title), row, ' ', border_fg, bg)

    def draw_button(self, col, row, w, label, fg=15, bg=4, pressed=False):
        final_fg = bg if pressed else fg
        final_bg = fg if pressed else bg
        self.put_cell(col, row, '[', 11, final_bg)
        c = col + 1
        pad = max(0, (w - 2 - len(label)) // 2)
        for _ in range(pad):
            self.put_cell(c, row, ' ', final_fg, final_bg)
            c += 1
        for ch in label:
            if c < col + w - 1:
                self.put_cell(c, row, ch, final_fg, final_bg)
                c += 1
        while c < col + w - 1:
            self.put_cell(c, row, ' ', final_fg, final_bg)
            c += 1
        self.put_cell(col + w - 1, row, ']', 11, final_bg)

    def draw_big_key(self, col, row, w, h, label, pressed=False, fg=15, bg=8):
        """Tegner en stor knapp med solid ramme og tydelig etikett."""
        final_fg = 0 if pressed else fg
        final_bg = 14 if pressed else bg
        border_col = 11 if pressed else 7

        # Tegn boks
        c2 = col + w - 1
        r2 = row + h - 1
        self.put_cell(col, row, 0xDA, border_col, final_bg)
        self.put_cell(c2,  row, 0xBF, border_col, final_bg)
        self.put_cell(col, r2,  0xC0, border_col, final_bg)
        self.put_cell(c2,  r2,  0xD9, border_col, final_bg)
        for c in range(col + 1, c2):
            self.put_cell(c, row, 0xC4, border_col, final_bg)
            self.put_cell(c, r2,  0xC4, border_col, final_bg)
        for r in range(row + 1, r2):
            self.put_cell(col, r, 0xB3, border_col, final_bg)
            self.put_cell(c2,  r, 0xB3, border_col, final_bg)
            for c in range(col + 1, c2):
                self.put_cell(c, r, ' ', final_fg, final_bg)

        # Sentrer tekst
        label_r = row + (h - 1) // 2
        label_c = col + max(1, (w - len(label)) // 2)
        for i, ch in enumerate(label):
            if label_c + i < c2:
                self.put_cell(label_c + i, label_r, ch, final_fg, final_bg)

    def render_to_framebuffer(self):
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


class TabOSSystem:
    def __init__(self, adb_device=ADB_DEVICE):
        self.adb_device = adb_device
        self.font = load_font()
        self.orientation = 'landscape'
        self.disp = DisplayEngine(self.font, self.orientation)
        
        # Visningsmodus: 'launcher', 'colortest', 'touchtest', 'keyboard'
        self.current_view = 'launcher'
        
        self.uptime_sec = 0
        self.start_time = time.time()
        self.status_msg = "tabOS v0.1 klar! Trykk med fingeren for a navigere."
        self.pressed_button_id = None
        self.dirty = True
        self.running = True
        
        # Touch-tilstand & statistikk
        self.touch_hw_x = 0
        self.touch_hw_y = 0
        self.touch_lcd_x = 0
        self.touch_lcd_y = 0
        self.touch_grid_c = 0
        self.touch_grid_r = 0
        self.is_down = False
        self.last_down = False
        self.touch_count = 0
        
        # Tegneflate for touch-test (sett med (col, row, color))
        self.draw_points = {}
        
        # Tekstfelt for tastatur-test
        self.typed_text = "Hei fra tabOS!"

    def toggle_orientation(self):
        if self.orientation == 'landscape':
            self.orientation = 'portrait'
        else:
            self.orientation = 'landscape'
        self.disp.set_orientation(self.orientation)
        self.dirty = True

    def hit_box(self, col, row, w, h, t_col, t_row):
        return (col <= t_col < col + w) and (row <= t_row < row + h)

    def render(self):
        self.disp.clear(0)
        self.uptime_sec = int(time.time() - self.start_time)

        if self.current_view == 'launcher':
            self.render_launcher()
        elif self.current_view == 'colortest':
            self.render_color_test()
        elif self.current_view == 'touchtest':
            self.render_touch_test()
        elif self.current_view == 'keyboard':
            self.render_keyboard_view()

        return self.disp.render_to_framebuffer()

    # -------------------------------------------------------------
    # 1. HOVEDMENY (LAUNCHER)
    # -------------------------------------------------------------
    def render_launcher(self):
        if self.orientation == 'landscape':
            # Header
            self.disp.draw_box(0, 0, 100, 3, "", 15, 4, 11)
            top_buf = f"tabOS BBS TERMINAL v0.1          [ NODE 1 ONLINE ]          UPTIME: {self.uptime_sec}s   BAT: 85%"
            self.disp.draw_string(2, 1, top_buf, 15, 4)

            # Banner
            banner = [
                " _       _     ___  ____  ",
                "| |_ ___| |__ / _ \\/ ___| ",
                "| __/ _ \\ '_ \\ | | \\___ \\ ",
                "| ||  __/ |_) | |_| |___) |",
                " \\__\\___|_.__/ \\___/|____/ "
            ]
            for i, line in enumerate(banner):
                self.disp.draw_string(36, 4 + i, line, 11, 0)
            self.disp.draw_string(27, 10, "=== CYBERDECK 7\" ALLWINNER A13 EDITION ===", 13, 0)

            # Kolonne 1: Online BBS
            self.disp.draw_box(4, 12, 28, 12, "BBS ONLINE PORTER", 15, 0, 14)
            self.disp.draw_button(6, 14, 24, " [1] TELEHACK BBS ", 15, 1, (self.pressed_button_id == 1))
            self.disp.draw_button(6, 17, 24, " [2] VERTRAUEN BBS", 15, 1, (self.pressed_button_id == 2))
            self.disp.draw_button(6, 20, 24, " [3] TITANTIC BBS ", 15, 1, (self.pressed_button_id == 3))

            # Kolonne 2: Tester & Verktøy
            self.disp.draw_box(36, 12, 28, 12, "VERKTOY & TEST", 15, 0, 10)
            self.disp.draw_button(38, 14, 24, " [4] RETRO SNAKE  ", 15, 2, (self.pressed_button_id == 4))
            self.disp.draw_button(38, 17, 24, " [5] TASTATUR-TEST", 15, 2, (self.pressed_button_id == 5))
            self.disp.draw_button(38, 20, 24, " [6] TOUCH-TEST   ", 15, 2, (self.pressed_button_id == 6))

            # Kolonne 3: Kontrollpanel & Grafikk
            self.disp.draw_box(68, 12, 28, 12, "KONTROLLPANEL", 15, 0, 13)
            self.disp.draw_button(70, 14, 24, " [7] ROTER SKJERM ", 15, 5, (self.pressed_button_id == 7))
            self.disp.draw_button(70, 17, 24, " [8] ANSI FARGETEST", 15, 5, (self.pressed_button_id == 8))
            self.disp.draw_button(70, 20, 24, " [X] NULLSTILL    ", 15, 5, (self.pressed_button_id == 9))

            # Status og Touch-bar
            self.disp.draw_box(0, 26, 100, 4, "", 15, 8, 7)
            self.disp.draw_string(2, 27, f"STATUS: {self.status_msg[:94]}", 11, 8)
            self.disp.draw_string(2, 28, "TOUCH : Trykk direkte pa knappene pa nettbrettet for a navigere!", 14, 8)

        else: # portrait
            # Header
            self.disp.draw_box(0, 0, 60, 3, "", 15, 4, 11)
            self.disp.draw_string(2, 1, "tabOS PORTRETT TERMINAL", 15, 4)
            self.disp.draw_string(40, 1, f"UPTIME: {self.uptime_sec}s", 14, 4)

            # Banner
            banner = [
                " _       _     ___  ____  ",
                "| |_ ___| |__ / _ \\/ ___| ",
                "| __/ _ \\ '_ \\ | | \\___ \\ ",
                "| ||  __/ |_) | |_| |___) |",
                " \\__\\___|_.__/ \\___/|____/ "
            ]
            for i, line in enumerate(banner):
                self.disp.draw_string(17, 4 + i, line, 11, 0)
            self.disp.draw_string(12, 10, "=== CYBERDECK PORTABLE BBS ===", 13, 0)

            # Hovedmeny knapper
            self.disp.draw_box(2, 12, 56, 17, "HOVEDMENY", 15, 0, 14)
            self.disp.draw_button(5, 14, 50, "  [1] TELEHACK ONLINE BBS  ", 15, 1, (self.pressed_button_id == 1))
            self.disp.draw_button(5, 17, 50, "  [2] VERTRAUEN SYNCHRONET ", 15, 1, (self.pressed_button_id == 2))
            self.disp.draw_button(5, 20, 50, "  [5] TASTATUR-TEST (STORE TASTER) ", 15, 2, (self.pressed_button_id == 5))
            self.disp.draw_button(5, 23, 50, "  [6] TOUCH-TEST & TEGNEFLATE      ", 15, 2, (self.pressed_button_id == 6))
            self.disp.draw_button(5, 26, 50, "  [8] ANSI FARGE- & GRAFIKKTEST    ", 15, 5, (self.pressed_button_id == 8))

            # Statusboks
            self.disp.draw_box(2, 30, 56, 6, "TERMINAL STATUS", 15, 0, 10)
            self.disp.draw_string(4, 32, f"> {self.status_msg[:50]}", 10, 0)
            self.disp.draw_string(4, 33, "> Trykk pa en knapp for a starte!", 14, 0)

            # Hurtigknapper nederst
            self.disp.draw_box(0, 38, 60, 11, "HURTIGNAVIGASJON", 15, 0, 7)
            self.disp.draw_button(4, 41, 24, " [ ROTER SKJERM ] ", 15, 4, (self.pressed_button_id == 7))
            self.disp.draw_button(32, 41, 24, " [ TASTATUR ] ", 15, 2, (self.pressed_button_id == 5))
            self.disp.draw_button(4, 45, 24, " [ TOUCH-TEST ] ", 15, 6, (self.pressed_button_id == 6))
            self.disp.draw_button(32, 45, 24, " [ FARGETEST ] ", 15, 5, (self.pressed_button_id == 8))

    # -------------------------------------------------------------
    # 2. ANSI FARGE- OG GRAFIKKTEST
    # -------------------------------------------------------------
    def render_color_test(self):
        # Header
        w = 100 if self.orientation == 'landscape' else 60
        self.disp.draw_box(0, 0, w, 3, "", 15, 5, 11)
        self.disp.draw_string(2, 1, "ANSI 16-FARGE & CP437 GRAFIKKTEST", 15, 5)
        self.disp.draw_button(w - 18, 1, 16, " [< TILBAKE] ", 15, 1, (self.pressed_button_id == 'back'))

        if self.orientation == 'landscape':
            # Fargepalett 0..15 i to rader
            self.disp.draw_box(2, 4, 96, 6, "16-FARGERS PALETT (STANDARD + INTENSE)", 15, 0, 14)
            for i in range(8):
                # Standard (0..7)
                col = 4 + i * 11
                self.disp.draw_string(col, 6, f"{i:2d} {COLOR_NAMES[i][:4]}", 15, 0)
                for block in range(4):
                    self.disp.put_cell(col + 6 + block, 6, 0xDB, i, 0) # █
                # Lys/Intens (8..15)
                idx = i + 8
                self.disp.draw_string(col, 8, f"{idx:2d} {COLOR_NAMES[idx][:4]}", 15, 0)
                for block in range(4):
                    self.disp.put_cell(col + 6 + block, 8, 0xDB, idx, 0)

            # Gradienter og skyggebokser (░ ▒ ▓ █)
            self.disp.draw_box(2, 11, 46, 7, "SKYGGING & GRADIENTER (CP437)", 15, 0, 10)
            shades = [0xB0, 0xB1, 0xB2, 0xDB] # ░ ▒ ▓ █
            fade_colors = [(1, 9), (2, 10), (4, 12), (5, 13), (6, 14), (3, 11)]
            for row_idx, (dark_c, light_c) in enumerate(fade_colors[:5]):
                r = 13 + row_idx
                self.disp.draw_string(4, r, f"FADE {row_idx+1}:", 15, 0)
                # Mørk fade opp til lys
                pos = 13
                for ch in shades:
                    self.disp.put_cell(pos, r, ch, dark_c, 0)
                    self.disp.put_cell(pos+1, r, ch, dark_c, 0)
                    pos += 2
                for ch in shades:
                    self.disp.put_cell(pos, r, ch, light_c, 0)
                    self.disp.put_cell(pos+1, r, ch, light_c, 0)
                    pos += 2
                self.disp.draw_string(pos + 2, r, "100%", light_c, 0)

            # CP437 Symboler, Rammer og Geometri
            self.disp.draw_box(50, 11, 48, 7, "BOKSER, RAMMER & RETRO SYMBOLER", 15, 0, 11)
            self.disp.draw_string(52, 13, "Enkel : ┌───┬───┐ ├───┼───┤ └───┴───┘", 14, 0)
            self.disp.draw_string(52, 14, "Dobbel: ╔═══╦═══╗ ╠═══╬═══╣ ╚═══╩═══╝", 11, 0)
            self.disp.draw_string(52, 15, "Kort  : \x03 \x04 \x05 \x06  Piler: \x18 \x19 \x1A \x1B \x1D \x12", 12, 0)
            self.disp.draw_string(52, 16, "Musikk: \x0D \x0E \x0F  Ikoner: \x01 \x02 \x0B \x0C \x7F \x1E", 10, 0)
            self.disp.draw_string(52, 17, "Matte : \xF1 \xF2 \xF3 \xF6 \xF7 \xF8 \xFB \xFD", 13, 0)

            # ANSI Fargekontrast-matrise
            self.disp.draw_box(2, 19, 96, 6, "TEKSTKONTRAST MATRIX (FG PA BAKGRUNN)", 15, 0, 13)
            bg_samples = [0, 4, 2, 1, 5, 6, 7]
            for col_idx, bg_c in enumerate(bg_samples):
                c = 4 + col_idx * 13
                self.disp.draw_string(c, 21, f"BG {bg_c:2d}", 15, 0)
                self.disp.draw_string(c, 22, " HVIT ", 15, bg_c)
                self.disp.draw_string(c, 23, " GUL  ", 11, bg_c)

            # Bunnmeny
            self.disp.draw_box(0, 26, 100, 4, "", 15, 8, 7)
            self.disp.draw_button(4, 27, 20, " [< TILBAKE] ", 15, 1, (self.pressed_button_id == 'back'))
            self.disp.draw_button(28, 27, 20, " [ ROTER SKJERM ] ", 15, 4, (self.pressed_button_id == 'rot'))
            self.disp.draw_string(52, 27, "Allwinner A13 32-bit ARGB TrueColor Framebuffer OK", 14, 8)
            self.disp.draw_string(52, 28, "16 ANSI-farger m/ IBM CP437 standard VGA font", 10, 8)

        else: # portrait
            # Portrett-versjon av fargetesten (60 kolonner)
            self.disp.draw_box(2, 4, 56, 10, "16 ANSI FARGER", 15, 0, 14)
            for i in range(16):
                col = 4 + (i % 4) * 13
                row = 6 + (i // 4) * 2
                self.disp.draw_string(col, row, f"{i:2d}", 15, 0)
                for b in range(4):
                    self.disp.put_cell(col + 3 + b, row, 0xDB, i, 0)

            self.disp.draw_box(2, 15, 56, 8, "CP437 RAMMER & GRADIENTER", 15, 0, 11)
            self.disp.draw_string(4, 17, "Enkel : \xDA\xC4\xC4\xC4\xC2\xC4\xC4\xC4\xBF \xC0\xC4\xC4\xC4\xC1\xC4\xC4\xC4\xD9", 14, 0)
            self.disp.draw_string(4, 18, "Dobbel: \xC9\xCD\xCD\xCD\xCB\xCD\xCD\xCD\xBB \xC8\xCD\xCD\xCD\xCA\xCD\xCD\xCD\xBC", 11, 0)
            self.disp.draw_string(4, 19, "Skygge: \xB0\xB0\xB1\xB1\xB2\xB2\xDB\xDB (0% -> 100%)", 10, 0)
            self.disp.draw_string(4, 20, "Symbol: \x03 \x04 \x05 \x06 \x01 \x02 \x0D \x0E \x18 \x19 \x1A \x1B \xF1 \xF7", 13, 0)

            self.disp.draw_box(2, 24, 56, 14, "FARGE-GRADIENTER", 15, 0, 10)
            shades = [0xB0, 0xB1, 0xB2, 0xDB]
            colors = [(1, 9, "ROD"), (2, 10, "GRONN"), (4, 12, "BLA"), (6, 14, "CYAN"), (5, 13, "MAGENTA")]
            for idx, (c1, c2, name) in enumerate(colors):
                r = 26 + idx * 2
                self.disp.draw_string(4, r, f"{name:7s}:", 15, 0)
                p = 13
                for ch in shades:
                    self.disp.put_cell(p, r, ch, c1, 0)
                    self.disp.put_cell(p+1, r, ch, c1, 0)
                    p += 2
                for ch in shades:
                    self.disp.put_cell(p, r, ch, c2, 0)
                    self.disp.put_cell(p+1, r, ch, c2, 0)
                    p += 2

            # Knapper nederst
            self.disp.draw_box(0, 40, 60, 9, "KONTROLL", 15, 0, 7)
            self.disp.draw_button(4, 43, 24, " [< TILBAKE] ", 15, 1, (self.pressed_button_id == 'back'))
            self.disp.draw_button(32, 43, 24, " [ ROTER SKJERM ] ", 15, 4, (self.pressed_button_id == 'rot'))

    # -------------------------------------------------------------
    # 3. TOUCH-TEST & TEGNEFLATE
    # -------------------------------------------------------------
    def render_touch_test(self):
        w = 100 if self.orientation == 'landscape' else 60
        # Header
        self.disp.draw_box(0, 0, w, 3, "", 15, 2, 11)
        self.disp.draw_string(2, 1, "ZET6221 TOUCH-SENSOR TEST & TEGNEFLATE", 15, 2)
        self.disp.draw_button(w - 18, 1, 16, " [< TILBAKE] ", 15, 1, (self.pressed_button_id == 'back'))

        # Info HUD
        hud_h = 4
        self.disp.draw_box(2, 3, w - 4, hud_h, "SANNTIDS DATA", 15, 0, 14)
        down_str = "NEDE (TOUCH)" if self.is_down else "OPPE (FRI)"
        down_col = 10 if self.is_down else 7
        self.disp.draw_string(4, 4, f"STATUS: {down_str}", down_col, 0)
        self.disp.draw_string(28, 4, f"HW (960x640): X={self.touch_hw_x:3d} Y={self.touch_hw_y:3d}", 11, 0)
        self.disp.draw_string(4, 5, f"LCD (800x480): X={self.touch_lcd_x:3d} Y={self.touch_lcd_y:3d}", 14, 0)
        self.disp.draw_string(36, 5, f"GRID: Kol={self.touch_grid_c:2d} Rad={self.touch_grid_r:2d}", 13, 0)
        self.disp.draw_string(w - 24, 4, f"TRYKK: {self.touch_count}", 15, 0)

        # Tegneområde
        canvas_top = 8
        canvas_bottom = 25 if self.orientation == 'landscape' else 41
        canvas_w = w - 4
        canvas_h = canvas_bottom - canvas_top
        self.disp.draw_double_box(2, canvas_top, canvas_w, canvas_h, "TEGNEFELT - DRA FINGEREN HER", 15, 0, 10)

        # Tegn berøringspunkter
        for (col, row), color in self.draw_points.items():
            if 3 <= col < w - 3 and canvas_top < row < canvas_bottom - 1:
                self.disp.put_cell(col, row, 0xDB, color, 0)

        # Bunnknapper
        btn_y = 26 if self.orientation == 'landscape' else 43
        self.disp.draw_box(0, btn_y, w, 4 if self.orientation == 'landscape' else 7, "", 15, 8, 7)
        self.disp.draw_button(4, btn_y + 1, 20, " [< HOVEDMENY] ", 15, 1, (self.pressed_button_id == 'back'))
        self.disp.draw_button(28, btn_y + 1, 20, " [ TOM SKJERM ] ", 15, 3, (self.pressed_button_id == 'clear'))
        self.disp.draw_button(52, btn_y + 1, 20, " [ ROTER SKJERM ] ", 15, 4, (self.pressed_button_id == 'rot'))

    # -------------------------------------------------------------
    # 4. FORSTØRRET TOUCH-TASTATUR MED SKRIVEFELT
    # -------------------------------------------------------------
    def render_keyboard_view(self):
        w = 100 if self.orientation == 'landscape' else 60
        # Header
        self.disp.draw_box(0, 0, w, 3, "", 15, 4, 11)
        self.disp.draw_string(2, 1, "CYBERDECK TOUCH-TASTATUR (STORE TASTER)", 15, 4)
        self.disp.draw_button(w - 18, 1, 16, " [< TILBAKE] ", 15, 1, (self.pressed_button_id == 'back'))

        # Skrivefelt / Text Display
        text_box_h = 5
        self.disp.draw_box(2, 3, w - 4, text_box_h, "NOTATBLOKK - SKREVET TEKST", 15, 0, 14)
        display_txt = self.typed_text[- (w - 10):] + "_" # med markør
        self.disp.draw_string(4, 5, display_txt, 11, 0)
        self.disp.draw_string(4, 6, f"Tegn: {len(self.typed_text)} | Trykk pa tastene nedenfor for a skrive!", 14, 0)

        if self.orientation == 'portrait':
            # PORTRETT: 60 kolonner x 50 rader
            # Store taster: Hver tast er 5 kolonner bred x 3 rader høy! (40x48 piksler!)
            # 10 taster = 50 kolonner, plassert fra col 5
            base_col = 5
            key_w = 5
            key_h = 3

            # Rad 1: Tall (1..0) på rad 9
            nums = "1234567890"
            for i, ch in enumerate(nums):
                k_col = base_col + i * key_w
                is_p = (self.pressed_button_id == f"key_{ch}")
                self.disp.draw_big_key(k_col, 9, key_w, key_h, ch, is_p, 15, 8)

            # Rad 2: QWERTY på rad 13
            row2 = "QWERTYUIOP"
            for i, ch in enumerate(row2):
                k_col = base_col + i * key_w
                is_p = (self.pressed_button_id == f"key_{ch}")
                self.disp.draw_big_key(k_col, 13, key_w, key_h, ch, is_p, 14, 8)

            # Rad 3: ASDFGHJKL på rad 17 (9 taster, sentrert med col 7)
            row3 = "ASDFGHJKL"
            for i, ch in enumerate(row3):
                k_col = 7 + i * key_w
                is_p = (self.pressed_button_id == f"key_{ch}")
                self.disp.draw_big_key(k_col, 17, key_w, key_h, ch, is_p, 10, 8)

            # Rad 4: ZXCVBNM , . på rad 21
            row4 = ["Z", "X", "C", "V", "B", "N", "M", ",", "."]
            for i, ch in enumerate(row4):
                k_col = 7 + i * key_w
                is_p = (self.pressed_button_id == f"key_{ch}")
                self.disp.draw_big_key(k_col, 21, key_w, key_h, ch, is_p, 11, 8)

            # Rad 5: Kontrollknapper på rad 25 (3 rader høye)
            # SLETT (col 5..16, w=12)
            self.disp.draw_big_key(5, 25, 14, key_h, "SLETT", (self.pressed_button_id == "key_bksp"), 15, 1)
            # MELLOMROM (col 21..44, w=24)
            self.disp.draw_big_key(21, 25, 22, key_h, "MELLOMROM", (self.pressed_button_id == "key_space"), 15, 4)
            # TOM (col 45..54, w=10)
            self.disp.draw_big_key(45, 25, 10, key_h, "TOM", (self.pressed_button_id == "key_clear"), 15, 5)

            # Rad 6: Navigasjon på rad 30
            self.disp.draw_button(5, 30, 24, " [< TILBAKE] ", 15, 1, (self.pressed_button_id == 'back'))
            self.disp.draw_button(31, 30, 24, " [ ROTER SKJERM ] ", 15, 4, (self.pressed_button_id == 'rot'))

            # Statusboks nederst
            self.disp.draw_box(2, 34, 56, 14, "TASTATUR-STATUS", 15, 0, 10)
            self.disp.draw_string(4, 36, f"> Siste tast trykket: {self.status_msg}", 11, 0)
            self.disp.draw_string(4, 38, "> Tastestorrelse: 40 x 48 piksler", 14, 0)
            self.disp.draw_string(4, 40, "> Stor touch-flate for noyaktig skriving!", 10, 0)

        else:
            # LANDSKAP: 100 kolonner x 30 rader
            # Taster er 8 kolonner brede x 2 rader høye!
            base_col = 10
            key_w = 8
            key_h = 2

            # Tallrad (rad 9)
            nums = "1234567890"
            for i, ch in enumerate(nums):
                k_col = base_col + i * key_w
                is_p = (self.pressed_button_id == f"key_{ch}")
                self.disp.draw_big_key(k_col, 9, key_w, key_h, ch, is_p, 15, 8)

            # QWERTY (rad 12)
            row2 = "QWERTYUIOP"
            for i, ch in enumerate(row2):
                k_col = base_col + i * key_w
                is_p = (self.pressed_button_id == f"key_{ch}")
                self.disp.draw_big_key(k_col, 12, key_w, key_h, ch, is_p, 14, 8)

            # ASDFGHJKL (rad 15)
            row3 = "ASDFGHJKL"
            for i, ch in enumerate(row3):
                k_col = 14 + i * key_w
                is_p = (self.pressed_button_id == f"key_{ch}")
                self.disp.draw_big_key(k_col, 15, key_w, key_h, ch, is_p, 10, 8)

            # ZXCVBNM (rad 18)
            row4 = ["Z", "X", "C", "V", "B", "N", "M", ",", "."]
            for i, ch in enumerate(row4):
                k_col = 14 + i * key_w
                is_p = (self.pressed_button_id == f"key_{ch}")
                self.disp.draw_big_key(k_col, 18, key_w, key_h, ch, is_p, 11, 8)

            # Kontroller (rad 21)
            self.disp.draw_big_key(10, 21, 16, key_h, "SLETT", (self.pressed_button_id == "key_bksp"), 15, 1)
            self.disp.draw_big_key(28, 21, 38, key_h, "MELLOMROM", (self.pressed_button_id == "key_space"), 15, 4)
            self.disp.draw_big_key(68, 21, 12, key_h, "TOM", (self.pressed_button_id == "key_clear"), 15, 5)
            self.disp.draw_big_key(82, 21, 12, key_h, "ENTER", (self.pressed_button_id == "key_enter"), 15, 2)

            # Bunnmeny
            self.disp.draw_box(0, 25, 100, 5, "", 15, 8, 7)
            self.disp.draw_button(4, 26, 20, " [< TILBAKE] ", 15, 1, (self.pressed_button_id == 'back'))
            self.disp.draw_button(28, 26, 20, " [ ROTER SKJERM ] ", 15, 4, (self.pressed_button_id == 'rot'))
            self.disp.draw_string(52, 26, f"Skrevet: {self.typed_text[-40:]}", 11, 8)
            self.disp.draw_string(52, 27, "Store touch-knapper: 64x32 piksler per tast!", 14, 8)

    # -------------------------------------------------------------
    # TOUCH HENDELSESBEHANDLING
    # -------------------------------------------------------------
    def handle_touch(self, hw_x, hw_y, is_down):
        just_down = is_down and not self.last_down
        just_up = not is_down and self.last_down
        self.last_down = is_down

        self.touch_hw_x = hw_x
        self.touch_hw_y = hw_y
        self.is_down = is_down

        # Skaler fra Zet6221 digitizer (960x640) til LCD piksler (800x480)
        self.touch_lcd_x = int(hw_x * 800 / 960)
        self.touch_lcd_y = int(hw_y * 480 / 640)

        # Beregn tegnkoordinater (col, row)
        if self.orientation == 'landscape':
            col = self.touch_lcd_x // 8
            row = self.touch_lcd_y // 16
        else: # portrait (90 grader rotert)
            xv = self.touch_lcd_y
            yv = 799 - self.touch_lcd_x
            col = xv // 8
            row = yv // 16

        self.touch_grid_c = col
        self.touch_grid_r = row

        if just_down:
            self.touch_count += 1

        prev_button = self.pressed_button_id

        # Hvis vi er i TOUCH-TEST modus: legg til tegnepunkt i sanntid!
        if self.current_view == 'touchtest' and is_down:
            color = (self.touch_count % 14) + 1 # unngå svart
            self.draw_points[(col, row)] = color
            # tegn en 2x2 blokk for fyldigere strek
            self.draw_points[(col+1, row)] = color
            self.draw_points[(col, row+1)] = color
            self.draw_points[(col+1, row+1)] = color
            self.dirty = True

        # Håndter klikk avhengig av visning
        if self.current_view == 'launcher':
            self.touch_launcher(col, row, is_down, just_up)
        elif self.current_view == 'colortest':
            self.touch_color_test(col, row, is_down, just_up)
        elif self.current_view == 'touchtest':
            self.touch_touch_test(col, row, is_down, just_up)
        elif self.current_view == 'keyboard':
            self.touch_keyboard(col, row, is_down, just_up)

        if self.pressed_button_id != prev_button or just_up:
            self.dirty = True

    def touch_launcher(self, col, row, is_down, just_up):
        w = 100 if self.orientation == 'landscape' else 60
        if is_down:
            if self.orientation == 'landscape':
                if self.hit_box(6, 14, 24, 1, col, row):   self.pressed_button_id = 1
                elif self.hit_box(6, 17, 24, 1, col, row): self.pressed_button_id = 2
                elif self.hit_box(6, 20, 24, 1, col, row): self.pressed_button_id = 3
                elif self.hit_box(38, 14, 24, 1, col, row): self.pressed_button_id = 4
                elif self.hit_box(38, 17, 24, 1, col, row): self.pressed_button_id = 5
                elif self.hit_box(38, 20, 24, 1, col, row): self.pressed_button_id = 6
                elif self.hit_box(70, 14, 24, 1, col, row): self.pressed_button_id = 7
                elif self.hit_box(70, 17, 24, 1, col, row): self.pressed_button_id = 8
                elif self.hit_box(70, 20, 24, 1, col, row): self.pressed_button_id = 9
            else:
                if self.hit_box(5, 14, 50, 1, col, row):   self.pressed_button_id = 1
                elif self.hit_box(5, 17, 50, 1, col, row): self.pressed_button_id = 2
                elif self.hit_box(5, 20, 50, 1, col, row): self.pressed_button_id = 5
                elif self.hit_box(5, 23, 50, 1, col, row): self.pressed_button_id = 6
                elif self.hit_box(5, 26, 50, 1, col, row): self.pressed_button_id = 8
                elif self.hit_box(4, 41, 24, 1, col, row): self.pressed_button_id = 7
                elif self.hit_box(32, 41, 24, 1, col, row): self.pressed_button_id = 5
                elif self.hit_box(4, 45, 24, 1, col, row): self.pressed_button_id = 6
                elif self.hit_box(32, 45, 24, 1, col, row): self.pressed_button_id = 8

        elif just_up:
            clicked = self.pressed_button_id
            self.pressed_button_id = None

            if clicked == 1:
                self.status_msg = "Kobler til TELEHACK BBS (telehack.com:23) via USB-bridge..."
            elif clicked == 2:
                self.status_msg = "Kobler til VERTRAUEN BBS (vert.synchro.net:23)..."
            elif clicked == 3:
                self.status_msg = "Kobler til TITANTIC BBS (ttb.rgbbs.info:23)..."
            elif clicked == 4:
                self.status_msg = "Starter RETRO SNAKE spill... Klargjor rutenett."
            elif clicked == 5:
                # Åpne Tastatur-test
                self.current_view = 'keyboard'
                self.status_msg = "Tastatur-test aktivert."
            elif clicked == 6:
                # Åpne Touch-test
                self.current_view = 'touchtest'
                self.status_msg = "Touch-test og tegneflate aktivert."
            elif clicked == 7:
                self.toggle_orientation()
                nor_orient = "PORTRETT" if self.orientation == 'portrait' else "LANDSKAP"
                self.status_msg = f"Skjerm rotert til {nor_orient}!"
            elif clicked == 8:
                # Åpne ANSI fargetest
                self.current_view = 'colortest'
                self.status_msg = "Viser ANSI fargetest og CP437 grafikk-kapasitet."
            elif clicked == 9:
                self.status_msg = "Allwinner A13 1.2GHz | 512MB DDR3 | 800x480 TFT | I2C Zet6221 OK"

            print(f"[*] Berøring registrert -> Valg: {clicked} -> {self.status_msg}", flush=True)

    def touch_color_test(self, col, row, is_down, just_up):
        w = 100 if self.orientation == 'landscape' else 60
        btn_y = 27 if self.orientation == 'landscape' else 43
        if is_down:
            if self.hit_box(w - 18, 1, 16, 1, col, row): self.pressed_button_id = 'back'
            elif self.hit_box(4, btn_y, 20, 1, col, row): self.pressed_button_id = 'back'
            elif self.hit_box(28, btn_y, 20, 1, col, row): self.pressed_button_id = 'rot'
        elif just_up:
            clicked = self.pressed_button_id
            self.pressed_button_id = None
            if clicked == 'back':
                self.current_view = 'launcher'
                self.status_msg = "Tilbake til hovedmeny."
            elif clicked == 'rot':
                self.toggle_orientation()

    def touch_touch_test(self, col, row, is_down, just_up):
        w = 100 if self.orientation == 'landscape' else 60
        btn_y = 27 if self.orientation == 'landscape' else 44
        if is_down:
            if self.hit_box(w - 18, 1, 16, 1, col, row): self.pressed_button_id = 'back'
            elif self.hit_box(4, btn_y, 20, 1, col, row): self.pressed_button_id = 'back'
            elif self.hit_box(28, btn_y, 20, 1, col, row): self.pressed_button_id = 'clear'
            elif self.hit_box(52, btn_y, 20, 1, col, row): self.pressed_button_id = 'rot'
        elif just_up:
            clicked = self.pressed_button_id
            self.pressed_button_id = None
            if clicked == 'back':
                self.current_view = 'launcher'
                self.status_msg = "Tilbake til hovedmeny."
            elif clicked == 'clear':
                self.draw_points.clear()
                self.touch_count = 0
                self.status_msg = "Tegneflate tomt!"
            elif clicked == 'rot':
                self.toggle_orientation()

    def touch_keyboard(self, col, row, is_down, just_up):
        w = 100 if self.orientation == 'landscape' else 60

        # Sjekk faste knapper øverst
        if is_down:
            if self.hit_box(w - 18, 1, 16, 1, col, row):
                self.pressed_button_id = 'back'
                return

        if self.orientation == 'portrait':
            base_col = 5
            key_w = 5
            key_h = 3

            # Rad 1: 1..0 på rad 9
            nums = "1234567890"
            for i, ch in enumerate(nums):
                if self.hit_box(base_col + i * key_w, 9, key_w, key_h, col, row):
                    if is_down: self.pressed_button_id = f"key_{ch}"
                    elif just_up: self.type_char(ch)
                    return

            # Rad 2: QWERTY på rad 13
            row2 = "QWERTYUIOP"
            for i, ch in enumerate(row2):
                if self.hit_box(base_col + i * key_w, 13, key_w, key_h, col, row):
                    if is_down: self.pressed_button_id = f"key_{ch}"
                    elif just_up: self.type_char(ch)
                    return

            # Rad 3: ASDFGHJKL på rad 17
            row3 = "ASDFGHJKL"
            for i, ch in enumerate(row3):
                if self.hit_box(7 + i * key_w, 17, key_w, key_h, col, row):
                    if is_down: self.pressed_button_id = f"key_{ch}"
                    elif just_up: self.type_char(ch)
                    return

            # Rad 4: ZXCVBNM , . på rad 21
            row4 = ["Z", "X", "C", "V", "B", "N", "M", ",", "."]
            for i, ch in enumerate(row4):
                if self.hit_box(7 + i * key_w, 21, key_w, key_h, col, row):
                    if is_down: self.pressed_button_id = f"key_{ch}"
                    elif just_up: self.type_char(ch)
                    return

            # Rad 5: SLETT, MELLOMROM, TOM på rad 25
            if self.hit_box(5, 25, 14, key_h, col, row):
                if is_down: self.pressed_button_id = "key_bksp"
                elif just_up: self.type_backspace()
                return
            if self.hit_box(21, 25, 22, key_h, col, row):
                if is_down: self.pressed_button_id = "key_space"
                elif just_up: self.type_char(" ")
                return
            if self.hit_box(45, 25, 10, key_h, col, row):
                if is_down: self.pressed_button_id = "key_clear"
                elif just_up: self.typed_text = ""; self.status_msg = "Tekst tømt."
                return

            # Navigasjon rad 30
            if self.hit_box(5, 30, 24, 1, col, row):
                if is_down: self.pressed_button_id = 'back'
                elif just_up: self.current_view = 'launcher'
                return
            if self.hit_box(31, 30, 24, 1, col, row):
                if is_down: self.pressed_button_id = 'rot'
                elif just_up: self.toggle_orientation()
                return

        else: # landscape
            base_col = 10
            key_w = 8
            key_h = 2

            nums = "1234567890"
            for i, ch in enumerate(nums):
                if self.hit_box(base_col + i * key_w, 9, key_w, key_h, col, row):
                    if is_down: self.pressed_button_id = f"key_{ch}"
                    elif just_up: self.type_char(ch)
                    return

            row2 = "QWERTYUIOP"
            for i, ch in enumerate(row2):
                if self.hit_box(base_col + i * key_w, 12, key_w, key_h, col, row):
                    if is_down: self.pressed_button_id = f"key_{ch}"
                    elif just_up: self.type_char(ch)
                    return

            row3 = "ASDFGHJKL"
            for i, ch in enumerate(row3):
                if self.hit_box(14 + i * key_w, 15, key_w, key_h, col, row):
                    if is_down: self.pressed_button_id = f"key_{ch}"
                    elif just_up: self.type_char(ch)
                    return

            row4 = ["Z", "X", "C", "V", "B", "N", "M", ",", "."]
            for i, ch in enumerate(row4):
                if self.hit_box(14 + i * key_w, 18, key_w, key_h, col, row):
                    if is_down: self.pressed_button_id = f"key_{ch}"
                    elif just_up: self.type_char(ch)
                    return

            if self.hit_box(10, 21, 16, key_h, col, row):
                if is_down: self.pressed_button_id = "key_bksp"
                elif just_up: self.type_backspace()
                return
            if self.hit_box(28, 21, 38, key_h, col, row):
                if is_down: self.pressed_button_id = "key_space"
                elif just_up: self.type_char(" ")
                return
            if self.hit_box(68, 21, 12, key_h, col, row):
                if is_down: self.pressed_button_id = "key_clear"
                elif just_up: self.typed_text = ""; self.status_msg = "Tekst tømt."
                return

            if self.hit_box(4, 26, 20, 1, col, row):
                if is_down: self.pressed_button_id = 'back'
                elif just_up: self.current_view = 'launcher'
                return
            if self.hit_box(28, 26, 20, 1, col, row):
                if is_down: self.pressed_button_id = 'rot'
                elif just_up: self.toggle_orientation()
                return

        if just_up:
            self.pressed_button_id = None

    def type_char(self, ch):
        self.typed_text += ch
        self.status_msg = f"Tastetrykk: '{ch}'"
        print(f"[*] Skrevet: '{ch}' -> Total: \"{self.typed_text}\"", flush=True)

    def type_backspace(self):
        if self.typed_text:
            self.typed_text = self.typed_text[:-1]
            self.status_msg = "Slettet siste tegn."
            print(f"[*] Slett -> Total: \"{self.typed_text}\"", flush=True)

    def flush_screen(self):
        fb_data = self.render()
        tmp_file = os.path.join(os.path.dirname(__file__), 'temp_fb.bin')
        with open(tmp_file, 'wb') as f:
            f.write(fb_data)
        
        # Send buffer til Allwinner A13 fb0
        subprocess.run(
            ["adb", "-s", self.adb_device, "push", tmp_file, "/data/local/tmp/screen.bin"],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
        subprocess.run(
            ["adb", "-s", self.adb_device, "shell", "dd if=/data/local/tmp/screen.bin of=/dev/graphics/fb0 bs=1536000 count=1 2>/dev/null"],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )

def run_tabos():
    print("=" * 60, flush=True)
    print("       tabOS - Interaktivt Operativsystem for A13 Tablet       ", flush=True)
    print("=" * 60, flush=True)
    print("[*] Stopper Android rammeverk for eksklusiv skjermkontroll...", flush=True)
    subprocess.run(["adb", "-s", ADB_DEVICE, "shell", "stop"], check=False)
    
    os_sys = TabOSSystem(ADB_DEVICE)
    print(f"[*] Skjerm initialisert ({os_sys.orientation}). Runder forste skjermbilde...", flush=True)
    os_sys.flush_screen()
    print("[*] tabOS er na synlig pa den fysiske skjermen!", flush=True)

    print("[*] Kobler til Zet6221 touch-sensor (/dev/input/event2)...", flush=True)
    getevent_cmd = ["adb", "-s", ADB_DEVICE, "shell", "getevent -q /dev/input/event2"]
    touch_proc = subprocess.Popen(getevent_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    cur_x = 480
    cur_y = 320
    touch_down = False

    last_time_update = time.time()

    print("[*] tabOS KJORER! Trykk med fingeren pa knappene pa nettbrettet!", flush=True)
    print("    - [8] ANSI FARGETEST: Komplett fargetabell og CP437 grafikk", flush=True)
    print("    - [6] TOUCH-TEST: Sanntids koordinater og finger-tegning", flush=True)
    print("    - [5] TASTATUR-TEST: Forstørrede taster (40x48 px) med levende tekstfelt", flush=True)
    print("    - [7] ROTER SKJERM: Bytte mellom landskap og portrett", flush=True)

    try:
        event_queue = queue.Queue()

        def reader():
            while os_sys.running:
                line = touch_proc.stdout.readline()
                if not line:
                    break
                event_queue.put(line)

        t = threading.Thread(target=reader, daemon=True)
        t.start()

        while os_sys.running:
            got_event = False
            while not event_queue.empty():
                line = event_queue.get_nowait()
                parts = line.strip().split()
                if len(parts) >= 3:
                    try:
                        etype = int(parts[0], 16)
                        ecode = int(parts[1], 16)
                        evalue = int(parts[2], 16)
                    except ValueError:
                        continue

                    if etype == 3: # EV_ABS
                        if ecode == 0x30: # ABS_MT_TOUCH_MAJOR (finger down/up)
                            touch_down = (evalue > 0)
                        elif ecode == 0x35: # ABS_MT_POSITION_X (0..960)
                            cur_x = evalue
                        elif ecode == 0x36: # ABS_MT_POSITION_Y (0..640)
                            cur_y = evalue
                    elif etype == 1: # EV_KEY fallback
                        if ecode == 0x14a:
                            touch_down = (evalue == 1)
                    elif etype == 0 and ecode == 0: # SYN_REPORT
                        os_sys.handle_touch(cur_x, cur_y, touch_down)
                        got_event = True

            # Oppdater klokke/oppetid hvert 5. sekund i launcher
            now = time.time()
            if os_sys.current_view == 'launcher' and now - last_time_update >= 5.0:
                last_time_update = now
                os_sys.dirty = True

            if os_sys.dirty:
                os_sys.flush_screen()
                os_sys.dirty = False

            time.sleep(0.02) # 50Hz

    except KeyboardInterrupt:
        print("\n[*] Avslutter tabOS...", flush=True)
    finally:
        touch_proc.terminate()
        print("[*] tabOS avsluttet.", flush=True)

if __name__ == '__main__':
    run_tabos()
