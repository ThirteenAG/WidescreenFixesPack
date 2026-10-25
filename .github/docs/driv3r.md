<div align="center">

<img src="https://thirteenag.github.io/screens/driv3r/attract.png" width="760" alt="DRIV3R - Widescreen Fix">

**DRIV3R Widescreen Fix** improves widescreen support, corrects aspect ratio, HUD, FOV, and radar scaling, and includes quality-of-life improvements such as a free-look camera, modern controls, increased draw distance and functional turn indicators.

[Website](https://fusionfix.io/wfp#driv3r) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Driv3r.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/driv3r/gameplay.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen gameplay">

- **Resolution** - the resolution list always includes the highest resolutions at their highest refresh rate, and switching resolution no longer shows the game at the wrong size
- **Aspect Ratio** - corrected for ultrawide
- **Field of View** - corrected for widescreen using hor+ scaling
- **HUD / 2D Elements** - all 2D quads scaled and repositioned to match the active aspect ratio
- **Radar** - position and scale corrected for any aspect ratio, including ultrawide
- **Mouse** - read every frame, without the original delay and dropped movement, so the menu cursor and mouse look respond like raw input
<br clear="both">

---

## New Options

- **Skip Intro** - bypass startup intro movies
- **Draw Distance** - added an option to increase draw distance
- **Window Mode** - choose fullscreen, a framed window or borderless
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect
- **Frame Rate Limit** - even frame pacing at 60 FPS by default; Unlimited is the original behaviour
- **Anisotropic Filtering** - sharper textures at steep viewing angles, up to 16x
- **Xbox 360 Gamma** - a custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360
- **Clip Cursor** - Locks the mouse cursor to the game window
- **Modern Controls** - keyboard, mouse and Xbox controller bindings laid out like in modern games, used by new profiles and by Reset in the controls menu; the controller can also pause the game with Start
---

## Camera

<div align="center">

**Mouse/Stick Look** - orbit the camera around the vehicle with the mouse or the right stick, look back still works. Reset timeout, return speed, sensitivity, and vertical axis inversion are configurable on fusionfix.io, in FusionFixHub or in the game's menu.

<img src="https://thirteenag.github.io/screens/driv3r/demo.webp" alt="Demo">

</div>

---

## Turn Indicators

<div align="center">

<img src="https://thirteenag.github.io/screens/driv3r/indicators.gif" width="400" alt="Turn indicators">

**Turn indicators** activate automatically when making a turn. Manual keyboard controls are available on fusionfix.io or in FusionFixHub.

</div>

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/driv3r/main1.jpg" alt="Before 16:9"> | <img src="https://thirteenag.github.io/screens/driv3r/main2.jpg" alt="After 16:9"> |
| Original 16:9 output | Correct 16:9 aspect ratio with proper HUD scaling |

</div>

---

## Settings

Configure the fix in FusionFixHub or on fusionfix.io. The first launch creates `scripts/Driv3r.WidescreenFix.cfg`.

Window Mode offers Fullscreen, a fixed-size centered Windowed mode, and Borderless at the monitor resolution. Restart to change it. The game remembers the selected resolution and refresh rate.

The gameplay FOV multiplier can be changed while playing. Confine Cursor keeps the pointer inside the active game window and releases it when the game loses focus. Modern Controls applies to new profiles and when resetting the game's controls.

## Configuration

The added settings text is available in English, French, German, Italian and Spanish. Choose Text Language in FusionFixHub to override the automatic choice. If the game language cannot be identified, English is used.

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. If Plugins is unavailable in your game version, use FusionFixHub or the generated `.cfg` file.

## Installation

1. Download the `.zip` from the release page.
2. Extract the contents directly into the game folder - the same folder as `Driv3r.exe` (Update 2 is required).
3. Launch the game once to create the `scripts/Driv3r.WidescreenFix.cfg`.
4. Configure the fix in FusionFixHub or on fusionfix.io. Restart the game when an option requires it.

---

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
