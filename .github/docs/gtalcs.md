<div align="center">

<img src="https://thirteenag.github.io/screens/gtalcs/main2.jpg" width="760" alt="Grand Theft Auto: Liberty City Stories Widescreen Fix (PCSX2F)">

**Grand Theft Auto: Liberty City Stories Widescreen Fix (PCSX2F)** corrects widescreen aspect ratio and field of view. HUD scaling outside 16:9 and FMVs remain uncorrected.

[Website](https://thirteenag.github.io/wfp#gtalcs) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTALCS.PCSX2F.WidescreenFix/main.c)

</div>

---

## Fixes

- **Aspect ratio** — Correct scaling on ultrawide displays.
- **Field of view** — Adjust the field of view for widescreen.
- **Skip intro** — Skip startup intros.

## Limitations

- **HUD** — Not corrected by this fix (Ultrawide).
- **Videos** — Not corrected by this fix.

---

## Installation

1. Download the fix archive from this release.
2. Use [PCSX2 Fork With Plugins](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest).
3. Extract the plugin archive into the emulator folder, preserving its `PLUGINS` directory.
4. Set the emulator’s aspect ratio or window size, then launch a supported version of the game.

## Useful information

One version of the game is compatible, **SLUS-21423** (crc: **7EA439F5**).

## Known Issues

- Hud is not fixed at all for non 16:9 resolutions
