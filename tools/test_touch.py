"""
Test script to read touch events from tablet over ADB and print decoded coordinates.
"""
import subprocess
import sys
import re

def test_touch():
    print("[*] Starter touch-lytting på /dev/input/event2...")
    print("[*] Trykk på skjermen på nettbrettet for å se hendelser! (Ctrl+C for å avslutte)")
    
    cmd = ["adb", "-s", "20080411413fc082", "shell", "getevent -q /dev/input/event2"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    cur_x = None
    cur_y = None
    touch_down = False
    
    try:
        for line in proc.stdout:
            parts = line.strip().split()
            if len(parts) >= 3:
                etype = int(parts[0], 16)
                ecode = int(parts[1], 16)
                evalue = int(parts[2], 16)
                
                # EV_ABS = 3
                if etype == 3:
                    if ecode == 0x35: # ABS_MT_POSITION_X
                        cur_x = evalue
                    elif ecode == 0x36: # ABS_MT_POSITION_Y
                        cur_y = evalue
                # EV_KEY = 1
                elif etype == 1:
                    if ecode == 0x14a: # BTN_TOUCH
                        touch_down = (evalue == 1)
                # EV_SYN = 0
                elif etype == 0:
                    if ecode == 0: # SYN_REPORT
                        if cur_x is not None or cur_y is not None:
                            state = "DOWN" if touch_down else "MOVE/UP"
                            print(f"Touch Event -> X: {cur_x}, Y: {cur_y}, State: {state}")
    except KeyboardInterrupt:
        print("\nAvslutter...")
    finally:
        proc.terminate()

if __name__ == "__main__":
    test_touch()
