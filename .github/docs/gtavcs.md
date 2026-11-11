<div align="center">

<img src="https://thirteenag.github.io/screens/gtavcs/main2.jpg" width="760" alt="Grand Theft Auto: Vice City Stories Fusion Fix (PCSX2F)">

**Grand Theft Auto: Vice City Stories Fusion Fix (PCSX2F)** corrects widescreen aspect ratio, HUD scaling, and field of view, with modern controls and experimental PC controls and cheats.

[Website](https://thirteenag.github.io/wfp#gtavcs) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTAVCS.PCSX2F.FusionFix/main.cpp)

</div>

---

## Fixes

- **Aspect ratio** - follows the emulator's aspect ratio or window size, at any width; the game's own 16:9 HUD layout is used from 16:9 up. Force Aspect Ratio (FusionFixHub) sets a ratio instead.
- **Field of view** - Widens the gameplay view without stretching. Field of View (0.5–2.0) adjusts the gameplay view further without changing cutscenes.
- **HUD** - Keeps HUD elements in proportion at the screen edges. HUD Size changes their size. HUD Aspect Ratio (Auto, 4:3, 16:9, 21:9, 32:9 or a custom ratio in FusionFixHub) keeps the HUD inside a centered area on wide screens; Auto uses the whole screen.
- **Cutscenes** - always keep their 16:9 framing at any aspect ratio: nothing cut off, nothing stretched. Cutscene Borders chooses which bars cover the rest: Off, Letterbox (top and bottom, on screens narrower than 16:9), Pillarbox (sides, on wider screens) or Both. With the bars off the cutscene fills the screen. The bars slide in and out.
- **Light glows and reticle** - stay round at any aspect ratio.
- **Little Willie camera** - corrected camera distance for the Little Willie helicopter.
- **Vehicle lights** - lit headlights, tail, brake and reverse lights on car and bike bodies. Vehicle Lights is on by default and switches at once; the Vanilla preset turns it off.
- **PC controls** - [keyboard and mouse controls](https://youtu.be/EhDviiFHYkE) with mouse camera, rebindable actions, key pictures in prompts and keyboard cheats. A pad keeps working next to them: the device used last drives the game and its prompts.

## Options

- **Modern controls** - Use the triggers to drive, aim and shoot.
- **Frame rate** - 30 or 60 FPS (`MAIN.FPSLimit` in the cfg); 30 FPS by default.
- **Loading** - temporarily unthrottles the emulator during loading screens.
- **Skip intro** - skips the logo and intro movies at startup.
- **Button icons** - Xbox or Nintendo button pictures in help text and menu hints; Auto uses Xbox buttons; PlayStation keeps the game's own buttons. The emulator can't tell which pad you hold, so choose the set yourself.

![](https://github.com/ThirteenAG/WidescreenFixesPack/assets/4904157/3c1d73d5-a7b0-4def-96f0-c3a439fef078)

## Installation

1. Download the fix archive from this release.
2. Use [PCSX2F](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest) (PCSX2 Fork With Plugins; the fix does not work in regular PCSX2).
3. Extract the plugin archive into the emulator folder, preserving its `PLUGINS` directory.
4. Set the emulator’s aspect ratio or window size, then launch a supported version of the game.
5. If an older `PLUGINS/GTAVCS.PCSX2F.WidescreenFix.elf` or `PLUGINS/GTAVCS.PCSX2F.ImVehLM.elf` is still there, delete it. Both are part of this Fusion Fix now; while one of them is installed, the Fusion Fix stays off and says so on screen.

## Useful information

One version of the game is compatible, **SLUS-21590** (crc: **4F32A11F**).

Ultra-wide screenshot:
![](https://user-images.githubusercontent.com/4904157/165582107-e738c8e7-9393-4b89-975a-701a28a80453.png)

## Settings

The settings are stored in `PLUGINS/GTAVCS.PCSX2F.FusionFix.cfg`, which the fix creates on the first start. Change them in FusionFixHub, on fusionfix.io, or in the game: the Display menu has one **Plugins** entry that opens this fix's page. It has its preset (Vanilla, Recommended, Enhanced), frame rate, field of view, HUD size and aspect ratio, cutscene borders, fast loading, skip intro, vehicle lights, modern controls, button icons, keyboard and mouse controls with bindings, and Restore Defaults. A line under the menu describes the selected row. Menu changes apply at once and are saved when the menu closes; Skip Intro applies the next time the game starts, and the menu says so. Changes made on fusionfix.io or in FusionFixHub apply the next time the emulated game starts, or when a save state is loaded.

- **Presets** - Vanilla keeps the original look (cutscene letterbox, intros, normal loading, no vehicle lights); Recommended is the default, and Enhanced is the same. Applying a preset changes only the settings that differ between presets; Vanilla uses PlayStation icons; Recommended and Enhanced use Auto. Frame rate, field of view, HUD, controls, mouse sensitivity and bindings keep your choice.
- **Save states** - after loading a state, the fix reads its settings file again, so your current settings apply instead of the ones saved in the state. Skip intro keeps the value the game started with.

## Not covered

- **Resolution, anti-aliasing, window modes and Alt+Tab** - handled by the emulator.
- **Videos** - the only videos are the logo and intro movies at startup, which Skip Intro skips. They are decoded straight to the screen; PCSX2F's own FMV aspect ratio setting shows them at 4:3.

# Project2DFX

[Project2DFX](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/releases/tag/gtavcsps2) is available as a separate plugin.

<img src="https://user-images.githubusercontent.com/4904157/166120612-d05a7258-2187-488c-b408-087511bd93a8.png" width="652px" />

# Vehicle lights

![vehicle lights](https://github.com/ThirteenAG/WidescreenFixesPack/assets/4904157/08128cd6-5801-496d-8ef8-2f6d8f55fdeb)

## Presets and FusionFixHub

- FusionFixHub text: English, French, German, Italian and Spanish.
- The added in-game settings menu is in English only.
- D-pad patch failure notices support English, French, German, Italian and Spanish through Text Language; Automatic uses English.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
