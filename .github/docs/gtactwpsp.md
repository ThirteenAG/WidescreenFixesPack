<div align="center">

<img src="https://thirteenag.github.io/screens/gtactwpsp/main2.jpg" width="760" alt="Grand Theft Auto: Chinatown Wars Fusion Fix (PPSSPP)">

**Grand Theft Auto: Chinatown Wars Fusion Fix (PPSSPP)** corrects widescreen aspect, HUD and video scaling, and adds an intro skip, control remapping, an optional third-person camera, and radio and mission options for the game.

[Website](https://thirteenag.github.io/wfp#gtactwpsp) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTACTW.PPSSPP.FusionFix/main.cpp)

</div>

---

## Fixes

- **Aspect ratio and camera** — Wider screens keep the original vertical view and show more of the city at the sides. Screens narrower than 16:9 (4:3, 5:4) keep the full original width and show more above and below. Force Aspect Ratio lets you choose another display shape.
- **FOV factor** — Scales the gameplay view from 0.5 to 2.0; higher values show more of the city around you. Story scenes keep their own framing.
- **HUD** — In Auto the radar, clock, weapon, money and other HUD parts sit at the real screen edges with their original proportions; notifications and button help stay centred. A 4:3, 16:9, 21:9 or 32:9 HUD box keeps them in a centred area. HUD scale shrinks the HUD around its edge anchors (0.5 to 1.0). On narrow screens the HUD is scaled to the screen width, so nothing is cut off.
- **PDA, map, apartment, menus and minigames** — Keep their original proportions, centred, with black bars at the sides on wide screens and above and below on narrow ones. The map extends to the full screen width.
- **Story scenes** — The comic panels keep their proportions at every screen shape. Cutscene borders: Pillarbox keeps the comic panels and the in-engine shots in the original 16:9 frame on wider screens; Letterbox keeps them in the 16:9 frame with bars above and below on narrower screens; Both does both. Off shows the in-engine shots across the whole screen with the game's own cinematic bars at the screen edges (on narrow screens the subtitles then sit on the picture).
- **Movies** — Movies are shown whole, with black bars where the screen shape differs. They are made for the PSP screen, so there is no separate movie setting.

## Options

- **Skip intro** — End the startup logo movie once initial loading has finished. Required startup notices remain visible.
- **Modern controls** — Use the mapped R/L shoulder buttons for acceleration/fire and braking/aiming. PDA and minigame controls stay native.
- **Third-person camera** — Press D-pad Right to switch between cameras; use the right stick to adjust position and angle, D-pad Up to reset. The original cinematic camera is disabled.
- **Radio** — Enable the radio in emergency vehicles.
- **Fast loading** — Temporarily unthrottle the emulator during loading screens and PDA transitions.
- **Xin missions** — Make the Xin missions available in the replay board. This experimental override can cause bugs; use it after completing the other missions.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the directory containing PPSSPP’s `memstick` folder, preserving the folders in the archive.
3. Enable plugins in PPSSPP and keep the supplied `plugin.ini` next to the plugin.
4. Launch the game in PPSSPP once to create `memstick/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/GTACTW.PPSSPP.FusionFix.cfg`.
5. Change settings in FusionFixHub, on the website or in the `.cfg` file, then restart the emulated game.

## Settings

Changes apply the next time the emulated game starts. There is no settings page inside the game: its menus are PDA applications without a list the fix could add to, so use FusionFixHub, the website or the `.cfg` file.

**Vanilla** shows the intro, uses the original controls, keeps the radio and loading as in the original and uses the game's own cinematic bars (Letterbox). **Recommended** (the defaults) and **Enhanced** skip the intro, turn on modern controls, radio in all vehicles and fast loading, and use Both for cutscene borders. The third-person camera, FOV factor, HUD aspect ratio and HUD scale keep your values. The experimental Xin replay option is under Compatibility and is not changed by presets.

Setting texts: English, French, German, Italian and Spanish.

## PPSSPP settings

For Auto, set the display layout you want in PPSSPP (stretched to the window or screen); the fix follows its shape. Resolution, anti-aliasing, window size and controller bindings are PPSSPP's settings. Bind the right analog stick in PPSSPP's control mapping to use the third-person camera.

## Known limits

- The game runs at its original frame rate.
- Scripted camera shots during gameplay keep the game's own bars and use the widened view; cutscene borders apply to story scenes.
- Button prompts keep the PSP artwork.
- If the game version isn't supported, the fix isn't installed and says so in its log.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
