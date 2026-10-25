<div align="center">

<img src="https://thirteenag.github.io/screens/gtactwpsp/main2.jpg" width="760" alt="Grand Theft Auto: Chinatown Wars Fusion Fix (PPSSPP)">

**Grand Theft Auto: Chinatown Wars Fusion Fix (PPSSPP)** corrects widescreen aspect, HUD and video scaling, and adds an intro skip, control remapping, an optional third-person camera, and radio and mission options for the game.

[Website](https://thirteenag.github.io/wfp#gtactwpsp) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTACTW.PPSSPP.FusionFix/main.cpp)

</div>

---

## Fixes

- **Aspect ratio** — Widens the 3D view to match PPSSPP’s display shape without stretching. Force Aspect Ratio lets you choose another display shape.
- **HUD** — Keeps HUD elements in proportion at the screen edges, with notifications and button help centred. The radar stays square. PDA, menu, map, loading and apartment screens keep their original proportions, with side borders when needed.
- **Videos** — Bink movies keep their aspect ratio instead of stretching.

## Options

- **Skip intro** — End the startup logo movie once initial loading has finished. Required startup notices remain visible.
- **HUD scale** — Scale the whole gameplay HUD around its screen-edge anchors (0.5–1.0); PDA applications keep their native proportions.
- **Modern controls** — Use the mapped R/L shoulder buttons for acceleration/fire and braking/aiming. PDA and minigame controls stay native.
- **Third-person camera** — Press D-pad Right to switch between cameras; use the right stick to adjust position and angle, D-pad Up to reset. The original cinematic camera is disabled.
- **Radio** — Enable the radio in emergency vehicles.
- **Loading** — Temporarily unthrottle the emulator during loading screens and PDA transitions.
- **Xin missions** — Make the Xin missions available in the replay board. This experimental override can cause bugs; use it after completing the other missions.

## Not covered

- **Resolution, anti-aliasing, window modes, Alt+Tab** — Handled by PPSSPP.
- **HUD aspect ratio** — No setting; the HUD is laid out across the full screen width.
- **In-game settings menu** — None; use fusionfix.io or FusionFixHub.
- **Cutscenes** — No separate cutscene field of view or border option.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the directory containing PPSSPP’s `memstick` folder, preserving the folders in the archive.
3. Launch the game in PPSSPP once to create `memstick/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/GTACTW.PPSSPP.FusionFix.cfg`.
4. Change settings on fusionfix.io or in FusionFixHub, then restart the emulated game.

## Hub settings

Changes apply the next time the emulated game starts. An old `GTACTW.PPSSPP.FusionFix.ini` isn't used anymore and can be deleted. Choose Vanilla, Recommended or Enhanced in FusionFixHub; Vanilla disables intro skipping, modern controls, the third-person camera, radio changes and faster loading. The experimental Xin replay option is under Compatibility and is not changed by presets. Choose anti-aliasing in PPSSPP.

## Presets and FusionFixHub

- Presets: Vanilla turns off skip intro, modern controls, the third-person camera, radio in all vehicles and fast loading; Recommended (the defaults) and Enhanced turn them on. HUD scale stays 1.0.
- Setting texts: English, French, German, Italian and Spanish.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
