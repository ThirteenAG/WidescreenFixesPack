<div align="center">

<img src="https://thirteenag.github.io/screens/scdaps2/main2.jpg" width="760" alt="Splinter Cell: Double Agent Widescreen Fix (PCSX2F)">

**Splinter Cell: Double Agent Widescreen Fix (PCSX2F)** corrects widescreen aspect ratio, HUD scaling, and field of view. Co-op HUD, FMV, and loading-screen limitations are listed below.

[Website](https://thirteenag.github.io/wfp#scdaps2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellDoubleAgent.PCSX2F.WidescreenFix/main.c)

</div>

---

## Fixes

- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.

## Limitations

- **Videos** — Not corrected by this fix.

---

## Installation

1. Download the fix archive from this release.
2. Use [PCSX2 Fork With Plugins](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest).
3. Extract the plugin archive into the emulator folder, preserving its `PLUGINS` directory.
4. Set the emulator’s aspect ratio or window size, then launch a supported version of the game.

## Useful information

- Two versions of the game are compatible, **SLUS-21356** (crc: **C0498D24**) and **SLES-53827** (crc: **ABE2FDE9**).

![](https://i.imgur.com/swnrFGs.jpg)

## Known Issues

- HUD in Coop mode is partially broken
- FMVs are stretched, I couldn't find a way to fix them
- Loadscreens are stretched (same as above)
