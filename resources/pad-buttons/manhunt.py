"""Build Manhunt's RenderWare button atlas from the shared controller artwork."""
from pathlib import Path
import struct
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def chunk(kind, data):
    return struct.pack("<III", kind, len(data), 0x1803FFFF) + data


def build():
    atlas = Image.new("RGBA", (512, 1024))
    for row, family in enumerate(("xbox", "playstation", "nintendo")):
        image = Image.open(ROOT / "textures/RWGTA/ButtonIcons" / f"pad_{family}.png").convert("RGBA")
        atlas.alpha_composite(image.crop((0, 0, 512, 256)), (0, row * 256))
    pixels = atlas.tobytes("raw", "BGRA")
    header = struct.pack("<II32s32sIIHHBBBB", 8, 0x1102, b"buttons", b"", 0x500, 1, 512, 1024, 32, 1, 4, 0)
    native = chunk(0x15, chunk(1, header + struct.pack("<I", len(pixels)) + pixels) + chunk(3, b""))
    data = chunk(0x16, chunk(1, struct.pack("<HH", 1, 1)) + native + chunk(3, b""))
    output = ROOT / "data/Manhunt.FusionFix/scripts/Manhunt.FusionFix/buttons.txd"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)
    print(f"{output}: {len(data)} bytes")


if __name__ == "__main__":
    build()
