<div align="center">

<img src="https://thirteenag.github.io/screens/sniperelite/main2.jpg" width="760" alt="Sniper Elite Widescreen Fix">

**Sniper Elite Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#sniperelite) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SniperElite.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Any resolution, 0 = the desktop's.
- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/SniperElite.WidescreenFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

## Settings

FusionFixHub and fusionfix.io change the settings; the first launch creates `scripts/SniperElite.WidescreenFix.cfg`.
An old `scripts/SniperElite.WidescreenFix.ini` isn't used anymore and can be deleted.

- **Field of view** (0.5 to 2.0, default 1.0, applies at once): multiplies the gameplay field of view
  on top of the widescreen correction. Cutscenes keep their framing. The same value in every preset.
- **Resolution** (width and height; 0 = the desktop resolution).
- **Video mode**: Original keeps the whole picture with bars, Fill crops to cover the screen, and Stretch fills without preserving proportions. Changes apply during playback. Recommended and Enhanced use Fill; Vanilla uses Original.
- **Confine cursor** (on by default, applies at once): keeps the cursor inside the game window
  while the game is active; it is released when the game loses the focus, while the window is moved or a system menu
  is open, and at once when turned off.

## Known limits

- HUD correction uses a fixed centered 4:3 layout. HUD aspect selection, window modes, intro skipping, cutscene-border controls and replacement pad icons are not supported.

- The HUD stays at 4:3 in the middle of the screen.
- Window mode and startup intros remain controlled by the game.
- No in-game settings menu.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
