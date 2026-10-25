<div align="center">

<img src="https://thirteenag.github.io/screens/sh2/main2.jpg" width="760" alt="Silent Hill 2 Widescreen Fix">

**Silent Hill 2 Widescreen Fix** corrects widescreen scaling and field of view, with options for cutscene borders, frame rate, camera speed, lighting, and compatibility.

[Website](https://thirteenag.github.io/wfp#sh2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SilentHill2.WidescreenFix/dllmain.cpp)

</div>

---

## This project is a part of [Silent Hill 2: Enhanced Edition](https://enhanced.townofsilenthill.com/SH2/). It is recommended to use it instead.

> [!NOTE]
> Silent Hill 2: Enhanced Edition is the more complete option: it includes this fix's widescreen work and goes much further. This fix is kept as it is and only gets maintenance.

## Fixes

- **Resolution** — Set the output resolution; 0 uses the desktop resolution.
- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.
- **Videos** — Correct FMV scaling for widescreen.

## Options

- **Fullscreen videos** — Optionally make FMVs fill the screen.
- **Cutscene borders** — Letterbox keeps the bars of the original cutscenes, Off removes them (default).

### Compatibility and frame rate

- **CPU affinity** — Legacy compatibility option for audio looping on affected systems. Off by default; enable it only if needed. It restricts the game to one available CPU core.
- **Safe mode** — Prevent the game from reverting to default settings after closing.
- **Transitions** — Speed up screen transitions.
- **Frame rate** — Set the frame rate limit; increasing it may cause game bugs.

### Camera and effects

- **Camera speed** — Use PS2 camera speed with `FPSLimit = 30`.
- **Gamepad bindings** — Allow one button to be assigned to multiple actions.
- **Lighting** — Fix the flashlight and restore fog and lighting in an ending cutscene.
- **Noise effect** — Increase the noise effect's resolution.
- **Background images** — Crop letterboxing so background images fill the screen.

### Settings

All settings are stored in `scripts/SilentHill2.WidescreenFix.cfg`, which the fix writes with every setting and a comment above each on the first start; change settings on fusionfix.io or in FusionFixHub (English, French, German, Italian and Spanish). There is no in-game settings menu. The settings apply the next time the game starts. An old `scripts/SilentHill2.WidescreenFix.ini` isn't used anymore and can be deleted. The game is sharp on scaled desktops and gets the real desktop resolution.

## Known limits

- This fix does not offer adjustable field of view, HUD width, window mode, intro skipping or controller icons. For a broader update, see Silent Hill 2: Enhanced Edition.
- Single-core compatibility mode is optional and disabled by default. It does not repair the underlying timing or audio behavior.
- Fullscreen videos crop to fill the screen; there is no stretch mode.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/SilentHill2.WidescreenFix.cfg`.
4. Optionally change the settings on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
