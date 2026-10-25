<div align="center">

<img src="https://thirteenag.github.io/screens/larush/main2.jpg" width="760" alt="L.A. Rush Widescreen Fix">

**L.A. Rush Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling, field of view and video scaling for widescreen displays.

[Website](https://thirteenag.github.io/wfp#larush) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/LARush.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Renders at the desktop resolution, or at the resolution set in the fix's options (out-of-range values fall back to the desktop).
- **Aspect ratio** — Correct widescreen projection.
- **HUD and menus** — HUD graphics, text and menus keep their 4:3 proportions; the mouse cursor maps to them.
- **Videos** — Movies play in a 4:3 area instead of being stretched.
- **Startup** — The game's CD checks for its movies are handled, so the game runs from the hard disk.
- **Display scaling** — The game stays sharp on scaled (high-DPI) desktops.

## Options

- **Correct HUD and menus** (restart) — on in every preset.
- **Skip intro logos** (restart) — Vanilla off, Recommended and Enhanced on. Skips publisher, studio and technology logos and the legal notice. The story intro remains available and can be skipped normally.
- **Resolution width/height** (restart).

Settings are changed on fusionfix.io or in FusionFixHub; every option applies after a restart. Defaults are the Recommended preset; Vanilla and Enhanced are the other presets. No preset changes the resolution. The fix has no in-game menu. An old `plugins/LARush.WidescreenFix.ini` isn't used anymore and can be deleted.

## Known limits

- No field of view factor or HUD aspect setting; HUD is a centered 4:3 area.
- Window modes are the game's own; no controller icons.

## Installation

1. Download the fix archive from this release.
2. Extract it into the game folder, beside the game executable, keeping the folders in the archive.
3. Start the game once to create `plugins/LARush.WidescreenFix.cfg`.
4. Change settings on fusionfix.io or in FusionFixHub, then restart the game.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
