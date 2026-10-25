<div align="center">

<img src="https://thirteenag.github.io/screens/thematrixpathofneo/main2.jpg" width="760" alt="The Matrix: Path of Neo Widescreen Fix">

**The Matrix: Path of Neo Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling, field of view and video scaling for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thematrixpathofneo) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheMatrixPathOfNeo.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Renders at the desktop resolution, or at the resolution set in the fix's options (out-of-range values fall back to the desktop, then 640×480).
- **Aspect ratio** — Correct widescreen projection.
- **HUD and movies** — 2D graphics and the full-motion videos keep their aspect ratio; full-screen film effects still cover the screen.
- **Display scaling** — The game stays sharp on scaled (high-DPI) desktops.

## Options

- **Correct HUD and movies** (restart) — on in every preset.
- **Resolution width/height** (restart).

Settings are changed on fusionfix.io or in FusionFixHub; every option applies after a restart. Defaults are the Recommended preset; Vanilla and Enhanced are the other presets. No preset changes the resolution. The fix has no in-game menu. An old `scripts/TheMatrixPathOfNeo.WidescreenFix.ini` isn't used anymore and can be deleted. Supported executables: `Matrix3.exe` and `Matrix3-10.exe`.

## Known limits

- Field of view and HUD width cannot be adjusted. The HUD stays in a centered 4:3 area.
- No skip intro, window mode setting or controller icons.

## Installation

1. Download the fix archive from this release.
2. Extract it into the game folder, beside the game executable, keeping the folders in the archive.
3. Start the game once to create `scripts/TheMatrixPathOfNeo.WidescreenFix.cfg`.
4. Change settings on fusionfix.io or in FusionFixHub, then restart the game.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
