<div align="center">

<img src="https://thirteenag.github.io/screens/farcry/main2.jpg" width="760" alt="Far Cry Widescreen Fix">

**Far Cry Widescreen Fix** adds proper widescreen support, corrects aspect ratio, HUD, field of view and FMV playback, and adds optional post-processing effects.

[Website](https://thirteenag.github.io/wfp#farcry) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/FarCry.WidescreenFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/FarCry.WidescreenFix/Bin32/FarCry.WidescreenFix.ini)

</div>

---

## Fixes

- **Aspect Ratio** - corrected for all rendered elements
- **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (16:9, 21:9, or custom)
- **Field of View** - corrected for widescreen
- **FMVs** - fullscreen videos are scaled and centered at the correct aspect ratio
- **Text** - drawn without the widescreen HUD scaling, keeping it at its original proportions

---

## New Options

- **HUD Aspect Ratio Constraint** - constrains the HUD to an aspect ratio via the `HudAspectRatioConstraint` option (Auto, 16:9, 21:9, or a custom ratio)
- **FOV Factor** - makes the FOV higher or lower via the `FOVFactor` option, `1.0` by default
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect
- **Xbox 360 Gamma** - a custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/farcry/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/farcry/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |

</div>

# Ultrawide Screenshot

![scb](https://thirteenag.github.io/screens/farcry/ultrawide.jpg)

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `FarCry.exe` (use the `Bin32` files for the 32-bit version and the `Bin64` files for the 64-bit one).
3. Optionally edit `FarCry.WidescreenFix.ini` (`FarCry64.WidescreenFix.ini` for the 64-bit version) to configure the available options.
4. Launch the game.

> [!NOTE]
> Installing [SilentPatch for Far Cry](https://github.com/CookiePLMonster/SilentPatchFarCry/releases/latest) is strongly recommended - it's the perfect companion to this fix.

---

## Support the project

This project is only possible because of Fusion Fix sponsors. If you would like to see further updates and improvements, consider supporting it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>