<div align="center">

<img src="https://thirteenag.github.io/screens/farcry/main2.jpg" width="760" alt="Far Cry Widescreen Fix">

**Far Cry Widescreen Fix** adds proper widescreen support, corrects aspect ratio, HUD, field of view and FMV playback, and adds optional post-processing effects.

[Website](https://thirteenag.github.io/wfp#farcry) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/FarCry.WidescreenFix/dllmain.cpp)

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

- **HUD Aspect Ratio Constraint** - Keeps the HUD inside a centered 16:9, 21:9 or custom area. Auto uses the full screen.
- **Skip intro** - skip the five startup logo/legal movies while retaining story videos and normal startup checks. Enabled by default; changes require restart.
- **Movie framing** - Original keeps the picture’s proportions with black bars, Fill crops to fill the screen (the default), and Stretch fills the screen by changing its proportions. Changes apply while playing.
- **Window mode** - Choose Fullscreen, Windowed or Borderless while playing. Alt+Enter switches between fullscreen and your last window mode. Window placement is remembered.
- **Confine cursor** - Keeps the pointer inside the active game window and releases it when you switch away or turn the setting off. Changes apply while playing.
- **FOV Factor** - Widens or narrows the gameplay view; 1.0 is the default.
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

## Settings and hub

Open **Options → Plugins** for settings and per-plugin presets. FOV, HUD width, movie framing, window mode and cursor confinement change while the game runs. Use English for the in-game settings menu; text with unsupported characters may leave a page unavailable. Use FusionFixHub for localized settings.

Choose the settings language on fusionfix.io or in FusionFixHub. The first launch writes the settings file next to the fix. FOV factor, HUD aspect constraint, movie framing, window mode and cursor confinement apply while the game runs; the other options require a restart. An old `FarCry.WidescreenFix.ini` or `FarCry64.WidescreenFix.ini` isn't used anymore and can be deleted.

Texts are available in English, French, German, Italian and Spanish.

## Known limits

- All presets leave SMAA off; turn it on separately.
- The game's own cutscene borders are kept.

Movie framing also respects smaller videos embedded in menus. Original adds black bars, Fill crops the picture, and Stretch changes its proportions. Subtitles and other interface elements keep their normal placement.

Skip intro removes only the startup logos. Story videos, menu music and normal startup checks remain.

Window mode supports framed, borderless and fullscreen output. Framed placement is remembered, and the game restores the previous mode if a change fails.

Cursor confinement follows focus and releases the pointer when disabled. Compatibility can depend on other input wrappers installed alongside the fix.

The FOV multiplier follows the normal player view. Aiming, death views, camera overrides and scripted views keep their intended framing.

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `FarCry.exe` (use the `Bin32` files for the 32-bit version and the `Bin64` files for the 64-bit one).
3. Launch the game once to create the settings file next to the fix: `FarCry.WidescreenFix.cfg` for 32-bit or `FarCry64.WidescreenFix.cfg` for 64-bit.
4. Configure the fix on fusionfix.io or in FusionFixHub, then restart the game.

> [!NOTE]
> Installing [SilentPatch for Far Cry](https://github.com/CookiePLMonster/SilentPatchFarCry/releases/latest) is strongly recommended - it's the perfect companion to this fix.

---

## Support the project

This project is only possible because of Fusion Fix sponsors. If you would like to see further updates and improvements, consider supporting it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
