<div align="center">

<img src="https://thirteenag.github.io/screens/mafia/main2.jpg" width="760" alt="Mafia: The City of Lost Heaven Widescreen Fix">

**Mafia: The City of Lost Heaven Widescreen Fix** adds proper widescreen support, corrects aspect ratio, HUD, field of view and FMV playback, and includes a range of quality-of-life improvements.

[Website](https://thirteenag.github.io/wfp#mafia) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Mafia.FusionFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Mafia.WidescreenFix/scripts/Mafia.FusionFix.ini)

</div>

---

## Fixes

- **Aspect Ratio** - corrected for all rendered elements
- **HUD** - repositioned and scaled to match the active aspect ratio, with an option to keep the original 4:3 centered HUD
- **Field of View** - corrected for widescreen
- **FMVs** - fullscreen videos are scaled and centered at the correct aspect ratio
- **Shadow Flickering** - fixed the z-fighting of shadows
- **Map / Cutscene Borders / Text / Menus** - also scaled to match the active aspect ratio

---

## New Options

- **Draw Distance** - increases the draw distance; the new distance and the skip ranges are configurable in `Mafia.WidescreenFix.ini`
- **FPS Limit** - caps the frame rate via the `FPS Limit` option in `Mafia.WidescreenFix.ini`
- **Settings** - saves all game settings to `savegame\settings.bin` instead of the registry, fixing an issue when the settings are lost
- **Borderless Windowed** - makes windowed mode borderless
- **Stick Deadzones** - configurable deadzones for the left and right stick (otherwise the camera just spins on its own)
- **Xbox 360 Gamma** - a custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/mafia/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/mafia/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |

</div>

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `Game.exe`.
3. Optionally edit `Mafia.FusionFix.ini` and `Mafia.WidescreenFix.ini` to configure the available options.
4. Launch the game.

> [!NOTE]
> **Xbox 360 Gamma** and **SMAA** require the D3D8 to D3D9 wrapper, which is bundled and enabled by default in `d3d8.ini` (`UseD3D8to9=1`).

---

## Support the project

This project is only possible because of Fusion Fix sponsors. If you would like to see further updates and improvements, consider supporting it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>

