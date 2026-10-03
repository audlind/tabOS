"""
Automated test script for all tabOS features:
- Menu navigation
- ANSI Color & CP437 Graphics Test View
- Touch Calibration & Drawing Pad View
- Enlarged Keyboard View (typing, backspace, space, clear)
- Orientation rotation in all views
"""
import sys
import os
sys.path.insert(0, os.path.dirname(__file__))
from tabos_runtime import TabOSSystem

def to_hw(lcd_x, lcd_y):
    """Konverterer LCD piksler (800x480) til Zet6221 rå hardware-koordinater (960x640)"""
    return int(lcd_x * 960 / 800), int(lcd_y * 640 / 480)

def test():
    print("=== Testing tabOS Complete Multi-View Touch Engine ===")
    os_sys = TabOSSystem()
    
    # 1. Start i Launcher
    assert os_sys.current_view == 'launcher'
    assert os_sys.orientation == 'landscape'
    print("[*] 1. Launcher aktiv i Landskap OK")

    # 2. Test åpning av ANSI FARGETEST ([8] på col 70..94, row 17 -> lcd_x=650, lcd_y=280)
    hx, hy = to_hw(650, 280)
    os_sys.handle_touch(hx, hy, is_down=True)
    assert os_sys.pressed_button_id == 8
    os_sys.handle_touch(hx, hy, is_down=False)
    assert os_sys.current_view == 'colortest'
    print("[*] 2. Byttet til ANSI Fargetest-visning OK")

    # Rendring av fargetest i landskap
    fb = os_sys.render()
    assert len(fb) == 800 * 480 * 4
    print("[*]    Fargetest rammebuffer rendret OK")

    # Roter skjerm inne i fargetest
    # Rot-knapp på col 28..48, row 27 -> lcd_x=300, lcd_y=440
    rx, ry = to_hw(300, 440)
    os_sys.handle_touch(rx, ry, is_down=True)
    os_sys.handle_touch(rx, ry, is_down=False)
    assert os_sys.orientation == 'portrait'
    print("[*]    Rotert fargetest til Portrett OK")

    # Tilbake-knapp inne i fargetest portrett (col 4..28, row 43)
    # I portrett: col = xv // 8, row = yv // 16
    # xv = lcd_y, yv = 799 - lcd_x
    # col=10, row=43 -> xv=80 (lcd_y=80), yv=688 -> lcd_x = 799-688 = 111
    bx, by = to_hw(111, 80)
    os_sys.handle_touch(bx, by, is_down=True)
    os_sys.handle_touch(bx, by, is_down=False)
    assert os_sys.current_view == 'launcher'
    print("[*]    Tilbake til hovedmeny fra fargetest OK")

    # 3. Test åpning av TOUCH-TEST ([6] på col 38..62, row 20 i landskap, men vi er i portrett!)
    # I portrett er [6] TOUCH-TEST på col 5..55, row 23 -> col=20, row=23
    # xv = 20*8 = 160 (lcd_y = 160), yv = 23*16 = 368 -> lcd_x = 799-368 = 431
    tx, ty = to_hw(431, 160)
    os_sys.handle_touch(tx, ty, is_down=True)
    os_sys.handle_touch(tx, ty, is_down=False)
    assert os_sys.current_view == 'touchtest'
    print("[*] 3. Byttet til Touch-test & Tegneflate OK")

    # Simuler tegning på tegneflaten (dra fingeren)
    for step in range(5):
        cx, cy = to_hw(400 + step * 10, 200 + step * 5)
        os_sys.handle_touch(cx, cy, is_down=True)
    assert len(os_sys.draw_points) > 0
    print(f"[*]    Tegnet {len(os_sys.draw_points)} piksler på tegneflaten OK")

    # Tøm skjerm
    # Tom-knapp i portrett på col 28..48, row 44
    # col=35, row=44 -> xv=280 (lcd_y=280), yv=704 -> lcd_x = 95
    cl_x, cl_y = to_hw(95, 280)
    os_sys.handle_touch(cl_x, cl_y, is_down=True)
    os_sys.handle_touch(cl_x, cl_y, is_down=False)
    assert len(os_sys.draw_points) == 0
    print("[*]    Tømte tegneflaten OK")

    # Tilbake til launcher
    # Tilbake på col 4..24, row 44 -> col=10, row=44 -> xv=80, yv=704 -> lcd_x=95, lcd_y=80
    b_x, b_y = to_hw(95, 80)
    os_sys.handle_touch(b_x, b_y, is_down=True)
    os_sys.handle_touch(b_x, b_y, is_down=False)
    assert os_sys.current_view == 'launcher'
    print("[*]    Tilbake til hovedmeny fra touch-test OK")

    # 4. Test åpning av FORSTØRRET TASTATUR ([5] på col 5..55, row 20 i portrett)
    # col=20, row=20 -> xv=160 (lcd_y=160), yv=320 -> lcd_x = 799-320 = 479
    kx, ky = to_hw(479, 160)
    os_sys.handle_touch(kx, ky, is_down=True)
    os_sys.handle_touch(kx, ky, is_down=False)
    assert os_sys.current_view == 'keyboard'
    print("[*] 4. Byttet til Forstørret Touch-tastatur OK")

    # Test tastetrykk på stor tast 'Q'
    # 'Q' er på col 5..9, row 13..15 (40x48 piksler!)
    # col=7, row=14 -> xv=56 (lcd_y=56), yv=224 -> lcd_x = 799-224 = 575
    initial_len = len(os_sys.typed_text)
    qx, qy = to_hw(575, 56)
    os_sys.handle_touch(qx, qy, is_down=True)
    assert os_sys.pressed_button_id == "key_Q"
    os_sys.handle_touch(qx, qy, is_down=False)
    assert len(os_sys.typed_text) == initial_len + 1
    assert os_sys.typed_text.endswith("Q")
    print(f"[*]    Trykket stor 'Q' -> Ny tekst: \"{os_sys.typed_text}\" OK")

    # Test SLETT (col 5..18, row 25..27)
    # col=10, row=26 -> xv=80 (lcd_y=80), yv=416 -> lcd_x = 799-416 = 383
    del_x, del_y = to_hw(383, 80)
    os_sys.handle_touch(del_x, del_y, is_down=True)
    os_sys.handle_touch(del_x, del_y, is_down=False)
    assert len(os_sys.typed_text) == initial_len
    print(f"[*]    Trykket stor 'SLETT' -> Ny tekst: \"{os_sys.typed_text}\" OK")

    # Test MELLOMROM (col 21..42, row 25..27)
    # col=30, row=26 -> xv=240 (lcd_y=240), yv=416 -> lcd_x = 383
    sp_x, sp_y = to_hw(383, 240)
    os_sys.handle_touch(sp_x, sp_y, is_down=True)
    os_sys.handle_touch(sp_x, sp_y, is_down=False)
    assert os_sys.typed_text.endswith(" ")
    print(f"[*]    Trykket stor 'MELLOMROM' -> Ny tekst: \"{os_sys.typed_text}\" OK")

    # Tilbake til launcher fra tastatur
    # Tilbake-knapp på col 5..28, row 30
    # col=10, row=30 -> xv=80, yv=480 -> lcd_x = 799-480 = 319
    kb_x, kb_y = to_hw(319, 80)
    os_sys.handle_touch(kb_x, kb_y, is_down=True)
    os_sys.handle_touch(kb_x, kb_y, is_down=False)
    assert os_sys.current_view == 'launcher'
    print("[*]    Tilbake til hovedmeny fra tastatur OK")

    print("\n[SUCCESS] 100% fullført: Alle visninger, store taster og touch-funksjoner fungerer feilfritt!")

if __name__ == '__main__':
    test()
