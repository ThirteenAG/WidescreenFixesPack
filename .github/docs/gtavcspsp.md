<div align="center">

<img src="https://thirteenag.github.io/screens/gtavcspsp/main2.jpg" width="760" alt="Grand Theft Auto: Vice City Stories Fusion Fix (PPSSPP)">

**Grand Theft Auto: Vice City Stories Fusion Fix (PPSSPP)** adds HUD scaling, right-stick controls, and optional 60 FPS support, and fixes the Little Willie camera and cars disappearing from garages.

[Website](https://thirteenag.github.io/wfp#gtavcspsp) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTAVCS.PPSSPP.FusionFix/main.cpp)

</div>

---

## Fixes

- **Widescreen** - Matches the display shape selected in PPSSPP. You can also choose an aspect ratio in FusionFixHub.
- **Field of view** - Widens the gameplay view for your screen. Gameplay FOV ranges from 0.5 to 2.0. The gameplay FOV multiplier does not change cutscenes.
- **HUD** - Adjust the HUD, radar and multiplayer radar sizes separately. HUD Aspect Ratio keeps the HUD inside a centered 4:3, 16:9, 21:9 or 32:9 area, or uses the whole screen with Auto. Radar positions can be changed in FusionFixHub.
- **Menu artwork** - Keeps its proportions with black bars at the sides when needed.
- **Cutscene borders** - Choose Off, Letterbox, Pillarbox or Both. Letterbox adds top and bottom bars on narrower screens; Pillarbox adds side bars on wider screens.
- **Videos** - The logo and title videos keep their proportions, with black bars at the sides on wider screens and above and below on narrower ones.

- **Little Willie camera** - Corrects the camera distance for the Little Willie helicopter.
- **Garages** - Prevents saved cars from disappearing from garages.

## Options

- **Dual analog controls** - right-stick camera and aim.
- **Controls** - Modern Control Scheme changes the driving controls.
- **Frame rate** - 30 or 60 FPS (`MAIN.FPSLimit` in the cfg); 30 FPS by default. At 60 FPS, mission timers that depend on the frame rate are adjusted, and the concert mission stays at 30 FPS.
- **Loading** - temporarily unthrottles the emulator during loading (mode 2 also during fades).
- **Skip intro** - skips the startup intros.
- **PC cheats** - enable keyboard cheat codes with PC Cheats.
- **Vehicle lights** - lit headlights, tail, brake and reverse lights on car and bike bodies. Vehicle Lights is on by default; the Vanilla preset turns it off. A change applies the next time the emulated game starts.
- **Controller icons** - Auto (default) uses Xbox 360 artwork. Original PSP, Xbox 360, Xbox One, PlayStation 3/4/5, Nintendo Switch and Steam Deck sets are also available. Scroll through the configuration option in the game's Controls menu, or choose the set on the fix's Game page; both apply at once. Changes made on fusionfix.io or in FusionFixHub apply the next time the emulated game starts. The sets replace the US version's artwork; other versions keep the original icons.

![image](https://user-images.githubusercontent.com/4904157/211559646-964ac702-d4e5-45e7-ad26-acb7eb24bdf6.png)

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the directory containing PPSSPP's `memstick` folder, preserving the folders in the archive.
3. Launch the game in PPSSPP.
4. If an older `memstick/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix`, `GTAVCS.PPSSPP.ImVehLM` or `GTAVCS.PPSSPP.GamepadIcons` folder is still there, delete it. All three are part of this Fusion Fix now; while one of them is installed, the Fusion Fix stays off and its log says why.

## Settings

The fix creates `memstick/PSP/PLUGINS/GTAVCS.PPSSPP.FusionFix/GTAVCS.PPSSPP.FusionFix.cfg` on the first launch. Change them in FusionFixHub, on fusionfix.io, or in the game: the Display menu has one **Plugins** entry that opens this fix's page. It has its preset (Vanilla, Recommended, Enhanced), Display and Camera (HUD size, HUD aspect ratio, radar and multiplayer radar size, field of view, cutscene borders, video aspect ratio), Game (frame rate, fast loading, skip intro, vehicle lights, controller icons) and Restore Defaults. Menu changes are saved when the menu closes; skip intro and vehicle lights need a restart, as do changes made on fusionfix.io or in FusionFixHub. Use FusionFixHub or fusionfix.io for radar positions, aspect ratio, controls and PC cheats.

- **Presets** - Vanilla keeps the original look (HUD, radar and multiplayer radar at full size, cutscene letterbox, PSP controller icons, intros, normal loading, no vehicle lights); Recommended is the default; Enhanced adds 60 FPS.

After updating from an older release, reapply your preferences in FusionFixHub.

## Not covered

- **Resolution, anti-aliasing, window modes and Alt+Tab** - handled by PPSSPP.

# Project2DFX

<details>
  <summary><a href="https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/releases/tag/gtavcs">Project2DFX</a> is available as a separate plugin</summary>
  <img src="https://user-images.githubusercontent.com/4904157/156879759-6bd08507-f30b-4890-ae69-b0503b998aa6.png" width="652px" />
</details>

# Vehicle lights and controller icons

https://user-images.githubusercontent.com/4904157/218305955-3a8a244f-aae8-456b-a7ef-dac9d5ca7118.mp4

![](https://github.com/ThirteenAG/WidescreenFixesPack/assets/4904157/e3042763-6bf8-419c-ba3a-9bb4e2dfa419)

## Presets and FusionFixHub

- FusionFixHub text: English, French, German, Italian and Spanish.
- The added in-game settings menu is in English only.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
