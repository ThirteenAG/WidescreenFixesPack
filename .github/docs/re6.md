<div align="center">

<img src="https://thirteenag.github.io/screens/re6/main2.jpg" width="760" alt="Resident Evil 6 Fusion Fix">

**Resident Evil 6 Fusion Fix** expands local co-op viewports to fill the screen, adds ultrawide support, raises the variable frame rate cap, and includes startup and windowed-mode options.

[Website](https://thirteenag.github.io/wfp#re6) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ResidentEvil6.FusionFix/)

</div>

---

## Fixes

- **Ultrawide** — The game renders at the screen's own aspect; the HUD keeps its shape at any aspect, side elements go to the screen's edges, full-screen overlays (low health, partner waiting, pause, the low health frame) cover the whole screen.
- **Objective markers** — The 3D objective markers point at their targets at any resolution, also in split screen (the game's placement only worked at 1280x720).
- **Local co-op** — Each player gets half of the screen (top/bottom, or left/right).
- **Cutscenes** — Fill the screen, with optional letterbox and 16:9 pillarbox; subtitles stay inside the picture.
- **Film grain** — Keeps its size at high resolutions.
- **Camera** — No clipping into walls next to the camera at wide aspects.
- **Display modes** — No startup crash on monitors with 256 or more display modes; the resolution list keeps all of them.
- **Frame rate** — The **VARIABLE** frame rate option is capped at the chosen limit instead of 120.

## Options

- **HUD aspect ratio** — Auto (whole width) or a ratio such as 16:9.
- **Field of view** — Multiplier for normal and vehicle gameplay cameras; scripted, weapon and unknown camera types keep their native projection.
- **Cutscene borders** — Off, letterbox, pillarbox or both.
- **Frame rate limit** — Unlimited, refresh rate or a number.
- **Window mode** — Windowed or borderless when the game's full screen option is off.
- **Pause in background** — Keep the game running when its window isn't active.
- **Split screen** — Vertical (left/right) layout and swapped views.
- **Skip intro** — Skip the startup logos.
- **Low health / dying effects, objective marker** — Can be hidden.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).

---

The FOV multiplier applies to normal and vehicle cameras and is suppressed during cinematics. Other camera types keep their original framing.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/ResidentEvil6.FusionFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

![](https://github.com/ThirteenAG/WidescreenFixesPack/assets/4904157/47f7d7ba-adc8-45c1-a601-b80e3d7b9c42)

![](https://github.com/ThirteenAG/WidescreenFixesPack/assets/4904157/31c645bd-a501-4efc-960b-0dfa216d2007)

## Settings

FusionFixHub changes most settings while the game runs. An old `scripts/ResidentEvil6.FusionFix.ini` isn't used anymore and can be deleted. Videos and the resolution list are left to the game, which lists all display modes. There is no in-game settings menu, and controller icons are the game's own.

## Known limits

- Movie-framing, raw-mouse and replacement controller-icon controls are not supported.
- The display lists support at most 8192 modes, 256 distinct resolutions and 32 refresh rates.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
