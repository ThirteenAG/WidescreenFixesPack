"""Build Bully's native texture dictionary from the shared controller artwork."""
from io import BytesIO
from pathlib import Path
import struct
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
ICONS = (
    "a", "b", "x", "y", "lb", "rb", "lt", "rt", "ls", "rs", "back", "start",
    "up", "down", "left", "right", "dpad", "updown", "leftright", "lstick",
    "lstickupdown", "lstickleftright", "lstickup", "lstickdown", "rstick",
    "rstickupdown", "rstickleftright", "lbrb", "ltrt",
)


def word(value):
    return struct.pack("<I", value)


def string(value):
    data = value.encode("utf-8")
    return word(len(data)) + data


def build():
    types = ["NiSourceTexture", "NiStringExtraData", "NiIntegerExtraData", "NiPixelData"]
    strings = ["Filename", "NifPackMinorVersion"]
    blocks = []
    # DXT5, one mip level, one face. Source textures use embedded pixels.
    pixel_header = bytearray.fromhex(
        "0600000000ffffffff000000000100000000000400000004000000000013000000050000000000130000"
        "0005000000000013000000050000000000ffffffff01000000000000002000000020000000000000000004000001000000"
    )
    for family in ("xbox", "playstation", "nintendo"):
        atlas = Image.open(ROOT / "textures/RWGTA/ButtonIcons" / f"pad_{family}.png").convert("RGBA")
        for icon, name in enumerate(ICONS):
            if icon < 27:
                cell = atlas.crop(((icon % 8) * 64, (icon // 8) * 64, (icon % 8 + 1) * 64, (icon // 8 + 1) * 64))
            else:
                cell = Image.new("RGBA", (64, 64))
                for side, button in enumerate((4, 5) if icon == 27 else (6, 7)):
                    picture = atlas.crop((button * 64, 0, (button + 1) * 64, 64)).resize((32, 32), Image.Resampling.LANCZOS)
                    cell.alpha_composite(picture, (side * 32, 16))
            encoded = BytesIO()
            cell.save(encoded, format="DDS", pixel_format="DXT5")
            pixels = encoded.getvalue()[128:]
            assert len(pixels) == 4096
            first = len(blocks)
            index = len(strings)
            strings.append(f"wfp_{family}_{name}.tga")
            source = (word(0xFFFFFFFF) + word(2) + word(first + 1) + word(first + 2) + word(0xFFFFFFFF)
                + b"\0" + word(index) + word(first + 3) + word(6) + word(2) + word(3) + b"\1\0\0")
            header = pixel_header.copy()
            struct.pack_into("<III", header, 71, 64, 64, 0)
            struct.pack_into("<I", header, 83, len(pixels))
            blocks.extend((source, word(0) + word(index), word(1) + word(9), bytes(header) + pixels))
    count = len(blocks)
    data = (b"Gamebryo File Format, Version 20.3.0.9\n" + word(0x14030009) + b"\1" + word(0) + word(count)
        + struct.pack("<H", len(types)) + b"".join(map(string, types))
        + b"".join(struct.pack("<H", i % 4) for i in range(count))
        + b"".join(word(len(block)) for block in blocks)
        + word(len(strings)) + word(max(map(len, strings))) + b"".join(map(string, strings)) + word(0)
        + b"".join(blocks) + word(count // 4) + b"".join(word(i) for i in range(0, count, 4)))
    output = ROOT / "data/Bully.FusionFix/plugins/Bully.FusionFix/buttons.nft"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)
    print(f"{output}: {count // 4} textures, {len(data)} bytes")


if __name__ == "__main__":
    build()
