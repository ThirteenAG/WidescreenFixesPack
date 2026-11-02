"""Build a native menu movie from an original StartScreen or PauseMenu GFX.

Usage: python build.py ffdec.jar original.gfx output.gfx work-directory
Only the Settings initialization action is replaced; other tags are retained.
"""
import subprocess
import sys
from pathlib import Path

jar, original, output, work = map(Path, sys.argv[1:])
work.mkdir(parents=True, exist_ok=False)
subprocess.run(["java", "-jar", str(jar), "-export", "script", str(work / "export"), str(original)], check=True)
matches = []
for script in (work / "export/scripts").rglob("*.as"):
    text = script.read_text(encoding="utf-8")
    if "var szSetting = new Array();" in text and "mainButtonArray[3] = controls_button;" in text:
        matches.append((script, text))
assert len(matches) == 1, "Expected one native Settings initialization action"
script, text = matches[0]
target = work / "import" / script.relative_to(work / "export/scripts")
target.parent.mkdir(parents=True)
target.write_text(text + "\n" + Path(__file__).with_name("Plugins.as").read_text(encoding="utf-8"), encoding="utf-8")
output.parent.mkdir(parents=True, exist_ok=True)
subprocess.run(["java", "-jar", str(jar), "-onerror", "abort", "-importScript", str(original), str(output), str(work / "import")], check=True)
