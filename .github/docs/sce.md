<div align="center">

<img src="https://thirteenag.github.io/screens/sce/main2.jpg" width="760" alt="Splinter Cell: Essentials Fusion Fix (PPSSPP)">

**Splinter Cell: Essentials Fusion Fix (PPSSPP)** adds a widescreen camera and HUD, menus and movies in their own proportions, right-stick controls, analog movement speed, optional 60 FPS and faster loading.

[Website](https://thirteenag.github.io/wfp#sce) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellEssentials.PPSSPP.FusionFix/main.cpp)

</div>

---

## Features

- **Camera and FOV** — Wider screens keep the vertical field of view and show more at the sides; narrower screens keep the full width and show more above and below. The FOV factor scales the gameplay view from 0.5 to 2.0. Hacking, keypad, lock-picking, computer and optic cable close-ups keep the framing their overlays were made for, in the middle of the screen.
- **HUD** — Auto places the HUD at the screen edges without stretching it: the light and sound meters, life bar, weapon, ammunition, quick inventory and message icon on the right, the objective box on the left and Lambert's transmissions at the top left. A 4:3, 16:9, 21:9 or 32:9 HUD box keeps them in a centered area; boxes narrower than the game's own layout keep that layout. HUD scale sizes the HUD from 0.5 to 1.5 at its edge.
- **Menus and screens** — Menus, the pause menu, the OPSAT, the mission failed screen, loading screens, mission briefings and the camera gadget view keep their proportions in the middle of the screen. Full-width menu bands span the screen, and screen fades cover it.
- **Window changes** — The view and the HUD follow window resizing and switching to full screen at once.
- **Movies** — The startup and story movies are shown complete at their own proportions, with black bars at the sides on wide screens and above and below on narrow ones. The movies use the PSP's full 480×272 frame, so there is no separate movie zoom setting.
- **Cutscenes** — The story is told in movies; there are no cutscene border settings.
- **Controls** — The right stick turns the camera; map it in PPSSPP. Analog movement speed lets the left stick set the walking and running speed. Stick deadzone adjusts how far a stick moves before it responds.
- **Startup, loading and frame rate** — Skip Intro skips the first startup movie (the second one is required by the game's loader). Faster loading temporarily unthrottles PPSSPP during loading screens. 60 FPS is optional.

## Installation

1. Extract the fix archive beside PPSSPP’s `memstick` folder, preserving its folders.
2. Enable plugins in PPSSPP and keep the supplied `plugin.ini` beside the plugin.
3. Launch the game once to create `memstick/PSP/PLUGINS/SplinterCellEssentials.PPSSPP.FusionFix/SplinterCellEssentials.PPSSPP.FusionFix.cfg`.
4. Change settings in FusionFixHub, the website settings editor or the `.cfg` file.

## Settings

The game has no settings menu for plugins, so the fix is configured outside the game. Skip Intro, analog movement speed, 60 FPS, faster loading and stick deadzone require a restart of the emulated game.

- **Skip intro**, **Analog movement speed** and **Faster loading** — on or off.
- **Frame rate** — choose 30 or 60 FPS (`MAIN.FPSLimit` in the cfg).
- **Stick deadzone** — 0 to 0.95.
- **FOV factor** — 0.5 to 2.0; 1.0 keeps the game's view.
- **HUD aspect ratio** — Auto, or a box ratio from 4:3 to 32:9.
- **HUD scale** — 0.5 to 1.5.
- **Force aspect ratio** (file and editor only) — Auto follows PPSSPP's display; `W:H` or a decimal from 1:1 to 4:1 overrides it.

**Vanilla** turns off intro skipping, analog movement speed, faster loading and 60 FPS. **Recommended** (the defaults) and **Enhanced** turn them on. Deadzone, FOV, HUD and aspect choices keep your values.

Setting texts: English, French, German, Italian and Spanish.

## PPSSPP settings

For Auto aspect correction, turn on **Stretch** in PPSSPP's display layout settings so the picture fills the window or screen; the fix then follows its shape. PPSSPP's own save and load screens are drawn by the emulator and fill the window. Configure controller bindings, rendering resolution, anti-aliasing, window size and frame skipping in PPSSPP.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
