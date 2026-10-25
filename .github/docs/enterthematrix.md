<div align="center">

<img src="https://thirteenag.github.io/screens/enterthematrix/main2.jpg" width="760" alt="Enter the Matrix Widescreen Fix">

**Enter the Matrix Widescreen Fix** corrects HUD, text and movie placement for widescreen displays and lists every display mode in MatrixOptions.

[Website](https://thirteenag.github.io/wfp#enterthematrix) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/EnterTheMatrix.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — MatrixOptions lists every display mode; the game renders at the resolution saved in its own `MatrixConfig.ini` (the desktop resolution when none is saved).
- **Aspect ratio** — Correct widescreen projection.
- **HUD** — HUD graphics and text keep their aspect ratio; the elements at the screen sides move to the edges.
- **Videos** — Movies keep their 4:3 aspect ratio instead of being stretched.
- **DPI** — The game is sharp on high-DPI displays; MatrixOptions keeps Windows' own dialog scaling.

---

## Options

- **Correct HUD** (restart) — on in every preset.
- **HUD aspect ratio** (while playing) — Automatic (whole screen width), 4:3, 16:9, 21:9, 32:9 or a custom value; Automatic in every preset.

Settings are changed on fusionfix.io or in FusionFixHub; HUD aspect ratio applies while playing; changing HUD correction requires a restart. The fix has no in-game menu. An old `scripts/EnterTheMatrix.WidescreenFix.ini` isn't used anymore and can be deleted. Supported executables: `Matrix.exe`, `Matrix12.exe`, `MatrixOptions.exe`, `MatrixOptions12.exe`.

## Known limits

- Choose window mode, multisampling and W-buffer in MatrixOptions.
- Windowed mode can count the title bar and borders in the selected size. The image stretches if the window's inner area does not match that resolution.
- No field of view setting and no skip intro (the logo sequence is driven by the game's movie scripts).
- No controller icons.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/EnterTheMatrix.WidescreenFix.cfg`.
4. Change settings on fusionfix.io or in FusionFixHub, then restart the game.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
