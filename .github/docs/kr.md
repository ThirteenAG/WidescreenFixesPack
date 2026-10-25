<div align="center">

<img src="https://thirteenag.github.io/screens/kr/main2.jpg" width="760" alt="Knight Rider: The Game Widescreen Fix">

**Knight Rider: The Game Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#kr) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/KnightRider.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Renders at the desktop resolution, or at the resolution set in the fix's options. Higher resolutions such as 1440p and 4K are supported. Out-of-range values fall back to the desktop, then 640×480.
- **Aspect ratio and field of view** — Widens the view at the sides while keeping its original height.
- **HUD** — Kept at 4:3 proportions in a centered box; night vision, objective markers and dialog faces stay in place.
- **Display scaling** — Scaled desktops (up to 300 %) neither crop nor blur the picture.
- **Window** — The game window has no frame.

## Options

- **Field of view factor** (0.5–2.0, live) — multiplies the corrected field of view; 1.0 in every preset.
- **Resolution width/height** (restart) — 0 uses the desktop resolution.

Settings are changed on fusionfix.io or in FusionFixHub; options marked live apply while the game runs, the others after a restart. Defaults are the Recommended preset; Vanilla and Enhanced are the other presets. No preset changes the resolution. The fix has no in-game menu. An old `scripts/KnightRider.WidescreenFix.ini` isn't used anymore and can be deleted.

## Known limits

- The field of view factor also applies to scripted camera shots that use the same projection.
- The HUD is a centered 4:3 box; there is no HUD aspect setting.
- No window mode setting (the game runs exclusive fullscreen through DirectDraw), no skip intro, no controller icons.

## Installation

1. Download the fix archive from this release.
2. Extract it into the game folder, beside the game executable, keeping the folders in the archive.
3. Start the game once to create `scripts/KnightRider.WidescreenFix.cfg`.
4. Change settings on fusionfix.io or in FusionFixHub. Settings marked live apply while the game runs; the others after a restart.

## PCSX2F: Knight Rider Widescreen Fix

Compatible version: crc **989192FE**.

Corrects the camera, HUD, inset views, backgrounds and videos for widescreen. Widescreen support is on by default and follows the emulator's display shape. The PS2 version has no field-of-view multiplier, HUD aspect or size setting, intro skip, or choice of cutscene borders and movie framing.

1. Use [PCSX2 Fork With Plugins](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest).
2. Extract the plugin archive into the emulator folder, preserving its `PLUGINS` directory.
3. Set the emulator's aspect ratio or window size, then launch the game.
4. Change settings on fusionfix.io or in FusionFixHub; changes apply the next time the emulated game starts. Choose anti-aliasing in PCSX2.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
