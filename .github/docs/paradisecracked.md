<div align="center">

<img src="https://thirteenag.github.io/screens/paradisecracked/main2.jpg" width="760" alt="Paradise Cracked Widescreen Fix">

**Paradise Cracked Widescreen Fix** corrects resolution handling and aspect ratio for widescreen displays.

[Website](https://thirteenag.github.io/wfp#paradisecracked) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ParadiseCracked.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Renders at the desktop resolution, or at the resolution set in the fix's options; the game's 800x600 surfaces and viewports follow it.
- **Aspect ratio** — Correct widescreen aspect ratio for the 3D view.
- **Display scaling** — The game gets the full resolution on scaled desktops.

## Options

| Setting | Values | Default | Applies |
|---|---|---|---|
| Resolution width / height | 0 = desktop | 0 | restart |

Resolution is a player choice and has no preset value. Invalid values fall back to the desktop resolution, then 640x480.

## Known limits

- Configure the fix on fusionfix.io or in FusionFixHub.
- No field of view factor or HUD constraint; 2D screens are not constrained.
- Some front-end artwork keeps its original 800x600 size.
- The startup logos cannot be skipped with this fix.
- Movies and window modes are the game's own.
- On some systems the game crashes when loading a saved game; this also happens without the fix.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/ParadiseCracked.WidescreenFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it. An old `scripts/ParadiseCracked.WidescreenFix.ini` isn't used anymore and can be deleted.
