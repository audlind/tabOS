raw = r"""_______________________________/\\\______________/\\\\\__________/\\\\\\\\\\\___
 ______________________________\/\\\____________/\\\///\\\______/\\\/////////\\\_
  _____/\\\_____________________\/\\\__________/\\\/__\///\\\___\//\\\______\///__
   __/\\\\\\\\\\\__/\\\\\\\\\____\/\\\_________/\\\______\//\\\___\////\\\_________
    _\////\\\////__\////////\\\___\/\\\\\\\\\__\/\\\_______\/\\\______\////\\\______
     ____\/\\\________/\\\\\\\\\\__\/\\\////\\\_\//\\\______/\\\__________\////\\\___
      ____\/\\\_/\\___/\\\/////\\\__\/\\\__\/\\\__\///\\\__/\\\_____/\\\______\//\\\__
       ____\//\\\\\___\//\\\\\\\\/\\_\/\\\\\\\\\_____\///\\\\\/_____\///\\\\\\\\\\\/___
        _____\/////_____\////////\//__\/////////________\/////_________\///////////_____"""

lines = raw.splitlines()

# Version A: Exactly as pasted with underscores
print("--- Version A: With underscores ---")
for i, l in enumerate(lines):
    # Escape for C
    escaped = l.replace('\\', '\\\\').replace('"', '\\"')
    print(f'display_draw_string(col, row + {i}, "{escaped}", ANSI_YELLOW, ANSI_BLACK);')

# Version B: Underscores replaced by spaces (clean 3D letters)
print("\n--- Version B: Spaces instead of underscores ---")
for i, l in enumerate(lines):
    s = l.replace('_', ' ')
    escaped = s.replace('\\', '\\\\').replace('"', '\\"')
    print(f'display_draw_string(col, row + {i}, "{escaped}", ANSI_YELLOW, ANSI_BLACK);')
