"""Build native prompt textures from the repository's existing pad atlases."""
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[3]
output = root / "data/TheSaboteur.FusionFix/scripts/TheSaboteur.FusionFix/buttons"
cells = {
    "icn_button_down": 0, "icn_button_right": 1, "icn_button_left": 2, "icn_button_up": 3,
    "icn_button_accept": 0, "icn_button_cancel": 1,
    "icn_LB": 4, "icn_RB": 5, "icn_LT": 6, "icn_RT": 7,
    "icn_L3": 8, "icn_R3": 9, "icn_select_back": 10, "icn_start": 11,
    "icn_dpad_up": 12, "icn_dpad_down": 13, "icn_dpad_left": 14, "icn_dpad_right": 15,
    "icn_dpad": 16, "icn_dpad_updown": 17, "icn_dpad_leftright": 18,
    "icn_joystick_L": 19, "icn_joystick_R": 24,
}
for family, name in [(1, "xbox"), (2, "playstation"), (3, "nintendo")]:
    atlas = Image.open(root / ("textures/RWGTA/ButtonIcons/pad_" + name + ".png")).convert("RGBA")
    folder = output / str(family)
    folder.mkdir(parents=True, exist_ok=True)
    for native, cell in cells.items():
        x, y = cell % 8 * 64, cell // 8 * 64
        atlas.crop((x, y, x + 64, y + 64)).save(folder / ("FF_Saboteur_" + str(family) + "_" + native + ".dds"))
