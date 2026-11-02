<div align="center">

<img src="https://thirteenag.github.io/screens/driverpl/attract.png" width="760" alt="Driver: Parallel Lines - Fusion Fix">

**Driver: Parallel Lines Fusion Fix** improves ultrawide support, corrects aspect ratio, HUD, FOV, and radar scaling, and includes quality-of-life improvements such as free-look camera and functional turn indicators.

[Website](https://fusionfix.io/wfp#driverpl) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/DriverParallelLines.FusionFix/dllmain.cpp)

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

Open **Options → Plugins → Driver: Parallel Lines Fusion Fix** for the fix’s settings and presets. Plugins is also available through Options in the pause menu. Press **F1** for the selected option’s complete help; use **Up / Down** to scroll and **F1 / Esc** to return. Options that need a restart show a warning. Each plugin has its own presets and Restore defaults. Presets leave the refresh-rate list setting unchanged. Choose **More pages** or **Previous page** to move through the fix’s settings.

On first launch the fix writes `scripts/DriverParallelLines.FusionFix.cfg`. The settings can also be changed on fusionfix.io or in FusionFixHub. Defaults: free mouse look and automatic indicators on, field of view multiplier 1, highest refresh rate on, fullscreen, frame rate limit 60 FPS, SMAA on and Xbox 360 gamma off. With the mouse action map, free mouse look replaces the native Look Left/Right/Back actions; camera return controls require it. The controller action map keeps its native look controls.

SMAA requires the game's MSAA setting to be disabled.

- **Skip Intro** - bypass startup logos and notices while preserving story movies
- **Window Mode** - fullscreen, a centered framed window at the selected resolution, or borderless at the monitor resolution. Alt+Enter switches between fullscreen and the last window mode.
- **Frame Rate Limit** - choose 30–1000 FPS, Monitor or Unlimited. The default, Recommended and Enhanced use 60; Vanilla leaves the original uncapped rate. Higher rates can affect gameplay.
- **Videos** - Movies keep their intended proportions, including anamorphic videos. Original shows them as the game does, whole inside its centred 4:3 frame with black bars around them; Fill fits the whole movie to the screen. Nothing is cut off or stretched. Default/Recommended/Enhanced use Fill; Vanilla uses Original aspect.
- **HUD Aspect Ratio Constraint** - Auto uses the screen width. Choose an aspect ratio to keep side HUD elements inside a centered box.
- **Pause When Unfocused** - choose whether play pauses while another window is active.
- **Mouse Look Sensitivity** - multiply mouse camera movement while keeping the profile sensitivity as the base; applies live.
- **Invert Mouse Y** - reverse vertical mouse look relative to the game profile; applies live.
- **Raw Mouse Input** - apply native mouse counts directly for consistent camera movement.
- **Controller Icons** - Xbox, PlayStation or Nintendo artwork on native binding names. Auto uses Xbox. Native XInput controls remain in use; deadzone and vibration are configurable.
- **Check for Updates** - optional and off by default; an available release is announced in Plugins.
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect
- **Xbox 360 Gamma** - a custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360
- **Confine cursor** - Keeps the pointer inside the active game window and releases it when the game loses focus.
---

## Camera

<div align="center">

<img src="https://thirteenag.github.io/screens/driverpl/camera.gif" width="400" alt="Free-look camera">

**Mouse Look** - reset timeout and return speed are configurable in Options → Plugins, on fusionfix.io or in FusionFixHub.

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
3. Launch the game once to create `scripts/DriverParallelLines.FusionFix.cfg`.
4. Configure the fix in Options → Plugins, on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

## Known limits

- Story cinematics use prerecorded movies; there is no separate in-engine cutscene-border setting.
- Borderless uses the monitor resolution; the native Video page can continue to show the profile’s selected resolution.

---

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
