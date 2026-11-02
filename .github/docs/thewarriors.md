<div align="center">

<img src="https://thirteenag.github.io/screens/thewarriors/main2.jpg" width="760" alt="The Warriors Fusion Fix (PPSSPP)">

**The Warriors Fusion Fix (PPSSPP)** adds widescreen camera and HUD layouts, fitted movies, dual analog controls, optional 60 FPS and faster loading.

[Website](https://thirteenag.github.io/wfp#thewarriors) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheWarriors.PPSSPP.FusionFix/main.cpp)

</div>

---

## Features

- **Camera and FOV** — Wider screens keep the vertical field of view and show more at the sides; narrower screens keep the full width and show more above and below. The FOV factor scales all 3D views from 0.5 to 2.0.
- **HUD** — Auto places the name and health panel, the radar, tutorial and objective messages and the menu prompts at the screen edges, without stretching them. A 4:3, 16:9, 21:9 or 32:9 HUD box keeps them in a centered area; boxes narrower than the game's own layout keep that layout. On narrow screens the top and bottom rows also move to the screen edges.
- **Menus** — Menus and the pause screen keep their proportions; the pause screen stays centered.
- **Movies** — The startup and story movies are shown complete at their own proportions, with black bars at the sides on wide screens and above and below on narrow ones. The movies use the PSP's full 480×272 frame, so there is no separate movie zoom setting.
- **Cutscenes** — In-engine cutscenes keep the game's own letterbox bars and show more at the sides on wide screens.
- **Controls** — Dual analog controls use the right stick for the camera and keep the left stick on movement, together with the game's camera inversion option.
- **Startup, loading and frame rate** — Skip Intro skips the startup logos and the opening movie. Faster loading temporarily unthrottles PPSSPP during loading screens. 60 FPS is optional.

## Installation

1. Extract the fix archive beside PPSSPP’s `memstick` folder, preserving its folders.
2. Enable plugins in PPSSPP and keep the supplied `plugin.ini` beside the plugin.
3. Launch the game once to create `memstick/PSP/PLUGINS/TheWarriors.PPSSPP.FusionFix/TheWarriors.PPSSPP.FusionFix.cfg`.
4. Change settings in FusionFixHub, the website settings editor or the `.cfg` file, then restart the emulated game.

## Settings

All settings apply the next time the emulated game starts. The game has no settings menu for plugins, so the fix is configured outside the game.

- **Dual analog controls**, **Skip intro** and **Faster loading** — on or off. **Frame rate** selects 30 FPS or an experimental 60 FPS mode.
- **FOV factor** — 0.5 to 2.0; 1.0 keeps the game's angles.
- **HUD aspect ratio** — Auto, or a box ratio from 4:3 to 32:9.
- **Force aspect ratio** (file and editor only) — Auto follows PPSSPP's display; `W:H` or a decimal from 1:1 to 4:1 overrides it.

**Vanilla** turns off intro skipping, dual analog controls and faster loading. **Recommended** and **Enhanced** turn them on. Frame rate, FOV, HUD and aspect choices keep your values.

Setting texts and patch notices are available in English, French, German, Italian and Spanish. Text Language selects the patch notice language; Auto uses English.

## PPSSPP settings

For Auto aspect correction, turn on **Stretch** in PPSSPP's display layout settings so the picture fills the window or screen; the fix then follows its shape. HUD positions are set when HUD elements are created, so after resizing the window or switching to full screen some elements move to their new place at the next loading screen. Configure controller bindings, rendering resolution, anti-aliasing, window size and frame skipping in PPSSPP.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
