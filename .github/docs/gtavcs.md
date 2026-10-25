<div align="center">

<img src="https://thirteenag.github.io/screens/gtavcs/main2.jpg" width="760" alt="Grand Theft Auto: Vice City Stories Widescreen Fix (PCSX2F)">

**Grand Theft Auto: Vice City Stories Widescreen Fix (PCSX2F)** corrects widescreen aspect ratio, HUD scaling, and field of view, with modern controls and experimental PC controls and cheats.

[Website](https://thirteenag.github.io/wfp#gtavcs) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTAVCS.PCSX2F.WidescreenFix/main.cpp)

</div>

---

## Fixes

- **Aspect ratio** - follows the emulator's aspect ratio or window size, at any width; the game's own 16:9 HUD layout is used from 16:9 up.
- **Field of view** - Widens the gameplay view without stretching; cutscenes keep their 16:9 framing. Gameplay FOV (0.5–2.0) adjusts the view further without changing cutscenes.
- **HUD** - Keeps HUD elements in proportion at the screen edges. HUD Scale changes their size. HUD Aspect Ratio Constraint (Auto, 16:9, 21:9 or a custom ratio) keeps the HUD inside a centered area on wide screens; Auto uses the whole screen.
- **Cutscene borders** - Choose Off, Letterbox (top and bottom bars on screens narrower than 16:9), Pillarbox (side bars on wider screens) or Both. The bars slide in and out.
- **Little Willie camera** - corrected camera distance for the Little Willie helicopter.
- **PC controls** - [keyboard and mouse controls (beta, experimental)](https://youtu.be/EhDviiFHYkE) with mouse camera, rebindable actions and keyboard cheats.

## Options

- **Modern controls** - Use the triggers to drive, aim and shoot.
- **60 FPS** - off by default.
- **Loading** - temporarily unthrottles the emulator during loading screens.
- **Skip intro** - skips the startup intros.

![](https://github.com/ThirteenAG/WidescreenFixesPack/assets/4904157/3c1d73d5-a7b0-4def-96f0-c3a439fef078)

The vehicle-light add-on keeps the original artwork in its Vanilla preset. Recommended and Enhanced enable the replacement lights. Changing the add-on setting requires restarting the emulated game.

## Installation

1. Download the fix archive from this release.
2. Use [PCSX2 Fork With Plugins](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest).
3. Extract the plugin archive into the emulator folder, preserving its `PLUGINS` directory.
4. Set the emulator’s aspect ratio or window size, then launch a supported version of the game.

## Useful information

One version of the game is compatible, **SLUS-21590** (crc: **4F32A11F**).

Ultra-wide screenshot:
![](https://user-images.githubusercontent.com/4904157/165582107-e738c8e7-9393-4b89-975a-701a28a80453.png)

## Settings

The settings are stored in `PLUGINS/GTAVCS.PCSX2F.WidescreenFix.cfg`, which the fix creates on the first start. Change them in FusionFixHub, on fusionfix.io, or in the game: the Display menu has one **Plugins** entry that lists this fix first, then Improved Vehicle Lights when it is installed. The fix's page has its preset (Vanilla, Recommended, Enhanced), frame rate, HUD size, fast loading, cutscene borders, keyboard and mouse controls with bindings, and Restore Defaults. Menu changes apply at once and are saved when the menu closes; changes made on fusionfix.io or in FusionFixHub apply the next time the emulated game starts. Field of view, HUD aspect ratio, skip intro and modern controls are set on fusionfix.io or in FusionFixHub only.

- **Presets** - Vanilla keeps the original look (cutscene letterbox, intros, normal loading); Recommended is the default, and Enhanced is the same. Presets never change controls, mouse sensitivity or bindings.

After updating from an older release, reapply your preferences in FusionFixHub.

## Not covered

- **Resolution, anti-aliasing, window modes and Alt+Tab** - handled by the emulator.
- **Videos** - not aspect-corrected; Skip intro skips the startup videos.
- **Controller icons** - the game keeps its PS2 button art; with keyboard and mouse controls on, prompts show PC keys.

# Project2DFX

[Project2DFX](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/releases/tag/gtavcsps2) is available as a separate plugin.

<img src="https://user-images.githubusercontent.com/4904157/166120612-d05a7258-2187-488c-b408-087511bd93a8.png" width="652px" />

# ImVehLM (Improved Vehicle Lights Mod)

![imvehlm](https://github.com/ThirteenAG/WidescreenFixesPack/assets/4904157/08128cd6-5801-496d-8ef8-2f6d8f55fdeb)

Installation:
- Download and extract [PCSX2 Fork With Plugins](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest).
- Download and extract archive to the root directory, where exe is located (PLUGINS and TEXTURES folders).
- Enable **Game Settings -> Graphics -> Texture Replacement -> Load Textures**.
- Launch the game.

## Improved vehicle lights settings

Vehicle Lights is on by default. Change it on the Improved Vehicle Lights page under Plugins or in FusionFixHub, then restart the emulated game.

## Presets and FusionFixHub

- FusionFixHub text: English, French, German, Italian and Spanish.
- The added in-game settings menu is in English only.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
