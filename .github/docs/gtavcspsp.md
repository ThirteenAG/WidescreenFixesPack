<div align="center">

<img src="https://thirteenag.github.io/screens/gtavcspsp/main2.jpg" width="760" alt="Grand Theft Auto: Vice City Stories Widescreen Fix (PPSSPP)">

**Grand Theft Auto: Vice City Stories Widescreen Fix (PPSSPP)** adds HUD scaling, right-stick controls, and optional 60 FPS support, and fixes the Little Willie camera and cars disappearing from garages.

[Website](https://thirteenag.github.io/wfp#gtavcspsp) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTAVCS.PPSSPP.WidescreenFix/main.cpp)

</div>

---

## Fixes

- **Widescreen** - Matches the display shape selected in PPSSPP. You can also choose an aspect ratio in FusionFixHub.
- **Field of view** - Widens the gameplay view for your screen. Gameplay FOV ranges from 0.5 to 2.0. Restore Cutscene FOV keeps cutscenes at their original 16:9 framing.
- **HUD** - Adjust the HUD, radar and multiplayer radar sizes separately. HUD Aspect Ratio Constraint keeps the HUD inside a centered area, or uses the whole screen with Auto. Radar positions can be changed in FusionFixHub.
- **Menu artwork** - Keeps its proportions with black bars at the sides when needed.
- **Cutscene borders** - Choose Off, Letterbox, Pillarbox or Both. Letterbox adds top and bottom bars on narrower screens; Pillarbox adds side bars on wider screens.

- **Little Willie camera** - Corrects the camera distance for the Little Willie helicopter.
- **Garages** - Prevents saved cars from disappearing from garages.

## Options

- **Dual analog controls** - right-stick camera and aim.
- **Controls** - Modern Control Scheme changes the driving controls.
- **60 FPS** - off by default; mission timers that depend on the frame rate are adjusted, the concert mission stays at 30 FPS.
- **Loading** - temporarily unthrottles the emulator during loading (mode 2 also during fades).
- **Skip intro** - skips the startup intros.
- **PC cheats** - enable keyboard cheat codes with PC Cheats.

![image](https://user-images.githubusercontent.com/4904157/211559646-964ac702-d4e5-45e7-ad26-acb7eb24bdf6.png)

The vehicle-light add-on keeps the original artwork in its Vanilla preset. Recommended and Enhanced enable the replacement lights. Changing the add-on setting requires restarting the emulated game.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the directory containing PPSSPP's `memstick` folder, preserving the folders in the archive.
3. Launch the game in PPSSPP.

## Settings

The fix creates `memstick/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.cfg` on the first launch. Change them in FusionFixHub, on fusionfix.io, or in the game: the Display menu has one **Plugins** entry that lists this fix first, then Improved Vehicle Lights and Gamepad Icons when they are installed. The fix's page has its preset (Vanilla, Recommended, Enhanced), Display and Camera (HUD, radar and multiplayer radar size, field of view, cutscene borders, original cutscene FOV), Game (frame rate, fast loading, skip intro) and Restore Defaults. Menu changes are saved when the menu closes; skip intro needs a restart, as do changes made on fusionfix.io or in FusionFixHub. Use FusionFixHub or fusionfix.io for HUD width, radar positions, aspect ratio, controls and PC cheats.

- **Presets** - Vanilla keeps the original look (HUD, radar and multiplayer radar at full size, cutscene letterbox, intros, normal loading); Recommended is the default; Enhanced adds 60 FPS.

After updating from an older release, reapply your preferences in FusionFixHub.

## Not covered

- **Resolution, anti-aliasing, window modes and Alt+Tab** - handled by PPSSPP.
- **Videos** - not aspect-corrected; Skip intro skips the startup videos.
- **Controller icons** - the original PSP art by default; the Gamepad Icons plugin replaces it with Xbox, PlayStation, Nintendo Switch or Steam Deck sets.

# Project2DFX

<details>
  <summary><a href="https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/releases/tag/gtavcs">Project2DFX</a> is available as a separate plugin</summary>
  <img src="https://user-images.githubusercontent.com/4904157/156879759-6bd08507-f30b-4890-ae69-b0503b998aa6.png" width="652px" />
</details>

# ImVehLM (Improved Vehicle Lights Mod) & Gamepad Icons

https://user-images.githubusercontent.com/4904157/218305955-3a8a244f-aae8-456b-a7ef-dac9d5ca7118.mp4

Installation:
- Download and extract the archive to the directory, where PPSSPP's memory stick is located (File -> Open Memory Stick).
- Inside TEXTURES folder, rename ULUS10160 to the ID, that matches your game (ULUS10160, ULES00502, ULES00503, ULJM05297, ULJM05395, ULJM05884).
- **textures.zip** is the same between two plugins, overwrite it when installing both.
- Enable **Game Settings -> Tools -> Developer Tools -> Replace Textures**.
- Launch the game.

**Gamepad Icons**, allows to select various controller icon styles (Xbox 360, Xbox One, PS3, PS4, PS5, Nintendo Switch, Steam Deck).
Scroll through the configuration option in CONTROLS menu to switch between them.

![](https://github.com/ThirteenAG/WidescreenFixesPack/assets/4904157/e3042763-6bf8-419c-ba3a-9bb4e2dfa419)

## Improved vehicle lights settings

Vehicle Lights is on by default. Change it on the Improved Vehicle Lights page under Plugins or in FusionFixHub, then restart the emulated game.

## Gamepad icon settings

Gamepad Icons selects the icon set: original PSP (default), Xbox 360, Xbox One, PS3, PS4, PS5, Nintendo Switch or Steam Deck. Change it in the game's Controls menu, on its page under Plugins, on fusionfix.io or in FusionFixHub; changes outside the Controls menu need a restart.

## Presets and FusionFixHub

- FusionFixHub text: English, French, German, Italian and Spanish.
- The added in-game settings menu is in English only.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
