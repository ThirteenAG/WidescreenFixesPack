"""Build Driv3r's controller glyphs from the shared button artwork."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def build():
    atlas = Image.new("RGBA", (256, 512))
    for family, name in enumerate(("xbox", "playstation", "nintendo")):
        source = Image.open(ROOT / "textures/RWGTA/ButtonIcons" / f"pad_{name}.png").convert("RGBA")
        for icon in range(17):
            x, y = icon % 8 * 64, icon // 8 * 64
            cell = source.crop((x, y, x + 64, y + 64)).resize((32, 32), Image.Resampling.LANCZOS)
            atlas.alpha_composite(cell, (icon % 8 * 32, family * 128 + icon // 8 * 32))
    output = ROOT / "data/Driv3r.FusionFix/scripts/Driv3r.FusionFix/buttons.dds"
    output.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(output, format="DDS", pixel_format="DXT3")
    print(f"{output}: {output.stat().st_size} bytes")


if __name__ == "__main__":
    build()
