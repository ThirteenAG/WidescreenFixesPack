<div align="center">

<img src="https://thirteenag.github.io/screens/sonicheroes/main2.jpg" width="760" alt="Sonic Heroes Widescreen Fix">

**Sonic Heroes Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#sonicheroes) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SonicHeroes.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Set the output resolution on fusionfix.io or in FusionFixHub.
- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.

## Options

- **Window mode** — Fullscreen, Windowed (optionally resizable) or Borderless (cover the monitor, fit the aspect ratio, or the chosen resolution in a centered window). Sharp on scaled (high-DPI) desktops.
- **Field of view** — Multiplies the gameplay field of view (0.5 to 2.0, applies at once); the HUD is unchanged.
- **Shadows** — GameCube-sized character shadows and a 1024 shadow map (the original uses 256).
- **Lens flare, Extras windows, staff roll** — Scaled correctly for widescreen.
- **Object distance** — Shows objects from farther away, reducing pop-in.
- **Switch flash effect** — Original, or the PlayStation 2 versions of the castle/mansion switch flash.
- **Better frame pacing** — A frame limiter with steadier frame times (60 by default; the game speed is tied to it).
- **No frame skipping, ignore the mouse, restore demos** — On by default; quit dialog, loading screen timeout and subtitles can be turned off.
- **Confine cursor** — Keeps the cursor inside the active game window; released on focus loss, while moving the window, in system menus and immediately when turned off.

Settings are on fusionfix.io or in FusionFixHub (English, French, German, Italian and Spanish); there is no in-game settings menu. Field of view and confine cursor apply at once, the rest at the next start. Resolution, minimum aspect ratio, clip range and object-distance tuning are on the advanced pages; the limiter method can only be changed in the settings file.

## Known limits

- The HUD is scaled for widescreen but has no HUD aspect ratio setting.
- No controller icon or raw mouse changes; the game's own input is used.
- The startup logos and intro cannot be skipped with this fix.
- Window placement uses the primary monitor; choosing another monitor is not supported.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/SonicHeroes.WidescreenFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

## Notes

The fix has no in-game menu; configure it on fusionfix.io or in FusionFixHub.

<a href="https://github.com/sponsors/xan1242"><img src="https://img.shields.io/badge/Sponsor_xan1242_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="250"></a><br>

<a href="https://www.paypal.com/paypalme/xan1242"><img src="https://img.shields.io/badge/Donate_to_xan1242_on_PayPal-5c5c5c?style=for-the-badge&logo=paypal&logoColor=blue" width="250"></a><br>
