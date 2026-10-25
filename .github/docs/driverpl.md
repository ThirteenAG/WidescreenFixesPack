<div align="center">

<img src="https://thirteenag.github.io/screens/driverpl/attract.png" width="760" alt="Driver: Parallel Lines - Widescreen Fix">

**Driver: Parallel Lines Widescreen Fix** improves ultrawide support, corrects aspect ratio, HUD, FOV, and radar scaling, and includes quality-of-life improvements such as free-look camera and functional turn indicators.

[Website](https://fusionfix.io/wfp#driverpl) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/DriverParallelLines.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/driverpl/gameplay.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen gameplay">

- **Resolution** - improved the display of available resolutions in the main menu
- **Aspect Ratio** - corrected for ultrawide
- **Field of View** - corrected for widescreen using hor+ scaling
- **HUD / 2D Elements** - all 2D quads scaled and repositioned to match the active aspect ratio
- **Radar** - position and scale corrected for any aspect ratio, including ultrawide
<br clear="both">

---

## New Options

Open **Options → Plugins** for the fix’s settings and presets. Field of view and cursor confinement change while the game runs; the other options need a restart. Each plugin has its own presets and Restore defaults.

On first launch the fix writes `scripts/DriverParallelLines.WidescreenFix.cfg`. The settings can also be changed on fusionfix.io or in FusionFixHub. An old `scripts/DriverParallelLines.WidescreenFix.ini` isn't used anymore and can be deleted. Defaults: free mouse look and automatic indicators on, field of view multiplier 1, highest refresh rate on, fullscreen, frame rate limit 60 FPS, SMAA and Xbox 360 gamma off. Free mouse look disables the native Look Left/Right/Back actions; camera return controls require it.

SMAA requires the game's MSAA setting to be disabled.

- **Skip Intro** - bypass startup intro movies
- **Window Mode** - fullscreen, a framed window or a borderless window at the selected resolution; at the desktop resolution the window covers the screen
- **Frame Rate Limit** - 60 FPS by default. Vanilla leaves the original uncapped rate; Recommended and Enhanced use 60. Higher rates can affect gameplay.
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect
- **Xbox 360 Gamma** - a custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360
- **Confine cursor** - Keeps the pointer inside the active game window and releases it when the game loses focus.
---

## Camera

<div align="center">

<img src="https://thirteenag.github.io/screens/driverpl/camera.gif" width="400" alt="Free-look camera">

**Mouse Look** - reset timeout, return speed are configurable in Options → Plugins, on fusionfix.io or in FusionFixHub.

</div>

---

## Turn Indicators

<div align="center">

<img src="https://thirteenag.github.io/screens/driverpl/indicators.gif" width="400" alt="Turn indicators">

**Turn indicators** activate automatically when making a turn. Manual controls are available in Options → Plugins, on fusionfix.io or in FusionFixHub.

</div>

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/driverpl/main1.jpg" alt="Before 21:9"> | <img src="https://thirteenag.github.io/screens/driverpl/main2.jpg" alt="After 21:9"> |
| Original 21:9 output | Correct 21:9 aspect ratio with proper HUD and FOV |

</div>

---

**Confine cursor** is on by default and applies while the game runs. The pointer is released when the game loses focus, while the window is dragged, in system menus or when the option is turned off.

## Installation

1. Download the `.zip` from the release page.
2. Extract the contents directly into the game folder - the same folder as `DriverParallelLines.exe`.
3. Launch the game once to create `scripts/DriverParallelLines.WidescreenFix.cfg`.
4. Configure the fix in Options → Plugins, on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

## Known limits

- No HUD aspect-ratio, window-mode, video-framing or cutscene-border settings.
- All presets leave SMAA off; turn it on separately.

---

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
