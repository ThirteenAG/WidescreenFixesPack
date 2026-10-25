<div align="center">

<img src="https://thirteenag.github.io/screens/srs/main2.jpg" width="760" alt="Street Racing Syndicate Widescreen Fix">

**Street Racing Syndicate Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling, and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#srs) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/StreetRacingSyndicate.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Renders at the desktop resolution, or at the resolution set in the fix's options (out-of-range values fall back to the desktop).
- **Aspect ratio** — The 3D view and 2D layer use the screen's aspect.
- **HUD** — HUD proportions and the rear-view mirror position stay correct.
- **Field of view** — Hor+ for the gameplay camera.
- **Display scaling** — The game stays sharp on scaled (high-DPI) desktops.

## Options

- **Correct HUD** and **Correct field of view** (restart) — on in every preset.
- **Field of view factor** (0.5–2.0, live) — 1.0 in every preset; works with or without the Hor+ correction.
- **Resolution width/height** (restart).

Settings are changed on fusionfix.io or in FusionFixHub; options marked live apply while the game runs, the others after a restart. Defaults are the Recommended preset; Vanilla and Enhanced are the other presets. No preset changes the resolution. The fix has no in-game menu. An old `Bin/scripts/StreetRacingSyndicate.WidescreenFix.ini` isn't used anymore and can be deleted.

## Known limits

- The HUD stays in a centered 4:3 area; its width cannot be adjusted.
- No skip intro, window mode setting or controller icons.
- Videos are not corrected.
- On some systems the game crashes when quitting; this also happens without the fix.

## Installation

1. Download the fix archive from this release.
2. Extract it into the game folder, beside the game executable, keeping the folders in the archive.
3. Start the game once to create `Bin/scripts/StreetRacingSyndicate.WidescreenFix.cfg`.
4. Change settings on fusionfix.io or in FusionFixHub. Settings marked live apply while the game runs; the others after a restart.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
