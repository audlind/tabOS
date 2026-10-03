import sys
import subprocess
import os

def send_frame(bin_name):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    bin_path = os.path.join(script_dir, '..', bin_name)
    if not os.path.exists(bin_path):
        print(f"Feil: Finner ikke {bin_path}")
        return

    print(f"Laster opp {bin_name} til nettbrettet...")
    subprocess.run(["adb", "push", bin_path, f"/data/local/tmp/{bin_name}"], check=True)
    
    print("Skriver direkte til skjermen (/dev/graphics/fb0)...")
    cmd = f"cat /data/local/tmp/{bin_name} > /dev/graphics/fb0; cat /data/local/tmp/{bin_name} >> /dev/graphics/fb0"
    subprocess.run(["adb", "shell", cmd], check=True)
    print("Ferdig! Sjekk nettbrett-skjermen din.")

if __name__ == '__main__':
    mode = 'landscape'
    if len(sys.argv) > 1:
        mode = sys.argv[1].lower()
    
    if 'port' in mode:
        print("=== Bytter til PORTRETT-modus (480x800 -> 60x50 ruter) ===")
        send_frame("tabos_demo_portrait.bin")
    else:
        print("=== Bytter til LANDSKAP-modus (800x480 -> 100x30 ruter) ===")
        send_frame("tabos_demo_landscape.bin")
