"""Build a native menu movie from a StartScreen or PauseMenu GFX.

Usage: python build.py ffdec.jar source.gfx output.gfx work-directory
The source is the game's movie or one built before (its Plugins code is replaced). The Settings initialization action
gets Plugins.as appended, and the Settings page's arrow and row highlight sprites are exported for it ("ffArrow",
"ffRowHighlight"); other tags are retained.
"""
import re
import subprocess
import sys
from pathlib import Path

jar, source, output, work = map(Path, sys.argv[1:])
work.mkdir(parents=True, exist_ok=False)
marker = "var ffSettings = this;"


def ffdec(*arguments):
    subprocess.run(["java", "-jar", str(jar), "-onerror", "abort", *map(str, arguments)], check=True)


# The sprites the Plugins page attaches: the Difficulty row's arrow and the row highlight inside hMover
ffdec("-swf2xml", source, work / "movie.xml")
xml = (work / "movie.xml").read_text(encoding="utf-8")


def placed(name):
    found = re.search(r'<item type="PlaceObject\dTag"[^>]*characterId="(\d+)"[^>]*name="' + name + '"', xml)
    assert found, name
    return found.group(1)


def sprite_end(sprite):
    start = xml.index(f'spriteId="{sprite}"')
    depth, at = 0, xml.rfind("<item", 0, start)
    for tag in re.finditer(r"<item\b[^>]*?(/?)>|</item>", xml[at:]):
        if tag.group(0).startswith("</"):
            depth -= 1
        elif not tag.group(1):
            depth += 1
        if depth == 0:
            return at + tag.end()
    raise AssertionError(sprite)


arrow = placed("arrowLeftDifficulty")
mover = placed("hMover")
mover_body = xml[xml.index(f'spriteId="{mover}"'):sprite_end(mover)]
bar = re.search(r'characterId="(\d+)"[^>]*name="h"', mover_body).group(1)
if 'ffArrow' not in xml:
    exports = ""
    for sprite, name in ((arrow, "ffArrow"), (bar, "ffRowHighlight")):
        exports += (f'<item type="ExportAssetsTag" forceWriteAsLong="true"><tags><item>{sprite}</item></tags>'
                    f'<names><item>{name}</item></names></item>')
    end = max(sprite_end(arrow), sprite_end(bar))
    xml = xml[:end] + exports + xml[end:]
(work / "movie.xml").write_text(xml, encoding="utf-8")
ffdec("-xml2swf", work / "movie.xml", work / "exported.gfx")

ffdec("-export", "script", work / "export", work / "exported.gfx")
matches = []
for script in (work / "export/scripts").rglob("*.as"):
    text = script.read_text(encoding="utf-8")
    if "var szSetting = new Array();" in text and "mainButtonArray[3] = controls_button;" in text:
        matches.append((script, text))
assert len(matches) == 1, "Expected one native Settings initialization action"
script, text = matches[0]
if marker in text:
    text = text[:text.index(marker)].rstrip() + "\n"
target = work / "import" / script.relative_to(work / "export/scripts")
target.parent.mkdir(parents=True)
target.write_text(text + "\n" + Path(__file__).with_name("Plugins.as").read_text(encoding="utf-8"), encoding="utf-8")
output.parent.mkdir(parents=True, exist_ok=True)
ffdec("-importScript", work / "exported.gfx", output, work / "import")
