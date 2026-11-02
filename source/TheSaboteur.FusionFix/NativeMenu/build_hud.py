"""Compile the small AS2 extension loaded into the native CommonUI movie.

Usage: python build_hud.py ffdec.jar output.gfx work-directory
"""
import struct
import subprocess
import sys
from pathlib import Path

jar, output, work = map(Path, sys.argv[1:])
work.mkdir(parents=True, exist_ok=False)
original = work / "interface-base.gfx"
rect_bits = "10000" + "".join(format(value, "016b") for value in (0, 25600, 0, 14400))
rect_bits += "0" * (-len(rect_bits) % 8)
rect = int(rect_bits, 2).to_bytes(len(rect_bits) // 8, "big")
tags = struct.pack("<H", (12 << 6) | 2) + b"\x07\x00" + struct.pack("<HH", 1 << 6, 0)
body = rect + struct.pack("<HH", 30 << 8, 1) + tags
original.write_bytes(b"FWS\x08" + struct.pack("<I", len(body) + 8) + body)
subprocess.run(["java", "-jar", str(jar), "-export", "script", str(work / "export"), str(original)], check=True)
script = work / "export/scripts/frame_1/DoAction.as"
text = script.read_text(encoding="utf-8")
assert text.strip() == "stop();", "Expected the empty extension initialization"
target = work / "import/frame_1/DoAction.as"
target.parent.mkdir(parents=True)
target.write_text(text + "\n" + Path(__file__).with_name("HUD.as").read_text(encoding="utf-8"), encoding="utf-8")
output.parent.mkdir(parents=True, exist_ok=True)
subprocess.run(["java", "-jar", str(jar), "-onerror", "abort", "-importScript", str(original), str(output), str(work / "import")], check=True)
