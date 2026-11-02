"""Build Mafia's controller font atlas from the shared button artwork."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def build():
    atlas = Image.new("RGBA", (512, 1024))
    for row, family in enumerate(("xbox", "playstation", "nintendo")):
        source = Image.open(ROOT / "textures/RWGTA/ButtonIcons" / f"pad_{family}.png").convert("RGBA")
        atlas.alpha_composite(source.crop((0, 0, 512, 256)), (0, row * 256))
    output = ROOT / "data/Mafia.FusionFix/scripts/Mafia.FusionFix/buttons.dds"
    output.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(output, format="DDS", pixel_format="DXT5")
    print(f"{output}: {output.stat().st_size} bytes")


if __name__ == "__main__":
    build()
