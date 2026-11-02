<div align="center">

<img src="https://thirteenag.github.io/screens/fireteambravo2/main2.jpg" width="760" alt="SOCOM: U.S. Navy SEALs Fireteam Bravo 2 Fusion Fix (PPSSPP)">

**SOCOM: U.S. Navy SEALs Fireteam Bravo 2 Fusion Fix (PPSSPP)** adds widescreen camera and HUD layouts, a native settings menu, dual analog controls, controller artwork, frame pacing and faster loading.

[Website](https://thirteenag.github.io/wfp#fireteambravo2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SOCOM.FireteamBravo2.PPSSPP.FusionFix/main.cpp)

</div>

---

## Features

- **Camera and FOV** — Widescreen views preserve the vertical field of view and show more at the sides, with the terrain and objects there fully drawn. The normal gameplay FOV factor ranges from 0.5 to 2.0; aiming and scripted cameras retain their own zoom.
- **HUD** — Auto places side groups, including the compass, at the screen edges. A 4:3, 16:9, 21:9 or 32:9 HUD box keeps them in a centered area. HUD scale resizes text and artwork together. World markers remain aligned with their targets.
- **Menus and movies** — Menus, loading screens, the profile keyboard and scope masks keep their proportions; dialog dimming and fades cover the whole screen. Movies fit with the complete picture visible. The PSP movies use the native 480×272 canvas, so there is no separate movie zoom setting.
- **Movie fallback** — If movie fitting fails, a notice appears and the game uses its native video copy for the rest of the session.
- **Scripted views** — Cutscene borders can preserve the original framing on narrow screens, wide screens or both. Off allows the wider camera view.
- **Controls** — Dual analog controls separate movement and camera/aim. Stock Windows PPSSPP supports Xbox-compatible right sticks; custom right-analog mappings also work. Button artwork offers Auto/Xbox, original PlayStation and Nintendo styles.
- **Frame rate** — Choose 30 or 60 FPS. Faster loading remains unthrottled when enabled.
- **Startup and loading** — Skip Intro passes publisher titles and the start prompt and acknowledges startup notices with one confirmation. Profile creation and real choices remain available. Faster loading temporarily unthrottles PPSSPP and restores normal speed afterward.

## Installation

1. Extract the fix archive beside PPSSPP’s `memstick` folder, preserving its folders.
2. Enable plugins in PPSSPP and keep the supplied `plugin.ini` beside the plugin.
3. Launch the game once to create `memstick/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionFix/SOCOM.FireteamBravo2.PPSSPP.FusionFix.cfg`.
4. Use the in-game Plugins page, FusionFixHub, the website settings editor or the `.cfg` file to change options.

## In-game settings

During gameplay, press Start and choose **Plugins**. Left and Right change values, Cross selects a row, Triangle opens its help and Circle goes back. Help and preset descriptions use the game’s scrolling list. The language can follow the PSP system language or use English, French, German, Italian or Spanish.

Camera, aspect, HUD, borders, icons, frame rate and language changes apply immediately from Plugins. Intro skipping, dual analog controls and faster loading show a restart notice when changed. Restart the emulated game after editing the `.cfg` file or applying settings through the Hub or website. Failed saves remain pending and can be retried from the menu.

If the title data is not recognized, a notice appears and startup skipping is left to the game.

If the pause-menu layout cannot hold the Plugins row, the original menu remains available. Unsupported button artwork keeps the game's original icon; a notice reports either problem.

**Vanilla** keeps the original startup titles, controls and PSP artwork and uses 30 FPS. **Recommended** and **Enhanced** enable startup skipping, dual analog controls and faster loading, use Xbox artwork and select 60 FPS. Other settings retain your values.

## PPSSPP settings

For Auto aspect correction, choose the desired display layout in PPSSPP. Force Aspect Ratio can override it with `W:H` or a decimal ratio. Configure controller bindings, rendering resolution, anti-aliasing, window size, display scaling and frame skipping in PPSSPP.

The plugin runs inside the PSP game. PPSSPP supplies desktop windowing, focus behavior and input mapping; desktop overlays and mouse bindings belong to the emulator.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
