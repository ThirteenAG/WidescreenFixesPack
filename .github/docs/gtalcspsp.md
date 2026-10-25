<div align="center">

<img src="https://thirteenag.github.io/screens/gtalcspsp/main2.jpg" width="760" alt="Grand Theft Auto: Liberty City Stories Widescreen Fix (PPSSPP)">

**Grand Theft Auto: Liberty City Stories Widescreen Fix (PPSSPP)** adds HUD scaling, right-stick controls, a modern control scheme, and optional 60 FPS and game loading speed adjustments.

[Website](https://thirteenag.github.io/wfp#gtalcspsp) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTALCS.PPSSPP.WidescreenFix/main.cpp)

</div>

---

## Fixes

- **Widescreen** - Matches the display shape selected in PPSSPP. You can also choose an aspect ratio in FusionFixHub.
- **Field of view** - Widens the gameplay view for your screen. Gameplay FOV ranges from 0.5 to 2.0. Restore Cutscene FOV keeps cutscenes at their original 16:9 framing.
- **HUD** - Adjust the HUD, radar and multiplayer radar sizes separately. HUD Aspect Ratio Constraint keeps the HUD inside a centered area, or uses the whole screen with Auto. Radar positions can be changed in FusionFixHub.
- **Menu artwork** - Keeps its proportions with black bars at the sides when needed.
- **Cutscene borders** - Choose Off, Letterbox, Pillarbox or Both. Letterbox adds top and bottom bars on narrower screens; Pillarbox adds side bars on wider screens.

## Options

- **Dual analog controls** - right-stick camera and aim.
- **Vehicle controls** - Modern Control Scheme swaps L/Square and R/Cross in vehicles; swap buttons and triggers in PPSSPP for trigger-based driving.
- **60 FPS** - off by default.
- **Loading** - temporarily unthrottles the emulator during loading screens.
- **Skip intro** - skips the startup intros.

The vehicle-light add-on keeps the original artwork in its Vanilla preset. Recommended and Enhanced enable the replacement lights. Changing the add-on setting requires restarting the emulated game.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the directory containing PPSSPP’s `memstick` folder, preserving the folders in the archive.
3. After the first launch, change settings on fusionfix.io, in FusionFixHub or in the game's menu.
4. Launch the game in PPSSPP.

# Project2DFX

<details>
  <summary><a href="https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/releases/tag/gtalcs">Project2DFX</a> is available as a separate plugin</summary>
  <img src="https://user-images.githubusercontent.com/4904157/156891784-3143e193-2e90-470d-bdc1-fdc94bf7ea87.png" width="652px" />
</details>

# ImVehLM (Improved Vehicle Lights Mod)

https://user-images.githubusercontent.com/4904157/218305955-3a8a244f-aae8-456b-a7ef-dac9d5ca7118.mp4

Installation:
- Download and extract the archive to the directory, where PPSSPP's memory stick is located (File -> Open Memory Stick).
- Inside TEXTURES folder, rename ULUS10041 to the ID, that matches your game (ULUS10041, ULES00151, NPJH50825, ULJM05255, ULJM05359, ULJM05885).
- Enable **Game Settings -> Tools -> Developer Tools -> Replace Textures**.
- Launch the game.

## Settings

The settings are stored in `memstick/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix/GTALCS.PPSSPP.WidescreenFix.cfg`, created on the first start. Change them in FusionFixHub, on fusionfix.io, or in the game: the Display menu has one **Plugins** entry that lists this fix first, then Improved Vehicle Lights when it is installed. The fix's page has its preset (Vanilla, Recommended, Enhanced), Display and Camera (HUD, radar and multiplayer radar size, field of view, cutscene borders, original cutscene FOV), Game and Controls (frame rate, fast loading, skip intro, modern controls) and Restore Defaults. Menu changes are saved when the menu closes; skip intro and modern controls need a restart, as do changes made on fusionfix.io or in FusionFixHub. The HUD aspect ratio, radar positions, aspect override and PC cheats are set on fusionfix.io or in FusionFixHub only.

- **Presets** - Vanilla keeps the original look (HUD, radar and multiplayer radar at full size, cutscene letterbox, intros and normal loading); Recommended is the default, and Enhanced is the same. Presets never change the control scheme.

After updating from an older release, reapply your preferences in FusionFixHub.

## Not covered

- **Resolution, anti-aliasing, window modes and Alt+Tab** - handled by PPSSPP.
- **Videos** - not aspect-corrected; Skip intro skips the startup videos.
- **Controller icons** - the game keeps its PSP button art.

## Improved vehicle lights settings

Vehicle Lights (on by default) is on the Improved Vehicle Lights page under Plugins and in FusionFixHub. Restart the emulated game after changing it.

## Presets and FusionFixHub

- FusionFixHub text: English, French, German, Italian and Spanish.
- The added in-game settings menu is in English only.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
