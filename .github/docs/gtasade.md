<div align="center">

<img src="https://thirteenag.github.io/screens/gtasade/main2.jpg" width="760" alt="Grand Theft Auto: San Andreas: The Definitive Edition Fusion Fix">

**Grand Theft Auto: San Andreas: The Definitive Edition Fusion Fix** adds a gameplay field-of-view multiplier, HUD and radar scaling, F5 quicksaves, startup skips, raw mouse input and mouse camera adjustments, and restores the PS2 and PC colour looks.

[Website](https://thirteenag.github.io/wfp#gtasade) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTASADE.FusionFix/dllmain.cpp)

</div>

---

## Configuration

Configure on fusionfix.io or in FusionFixHub. The first start creates `Gameface/Binaries/Win64/scripts/GTASADE.FusionFix.cfg`. An old `Gameface/Binaries/Win64/scripts/GTASADE.FusionFix.ini` isn't used anymore and can be deleted. The field of view applies while the game runs; the other settings need a restart.

Presets:
- **Vanilla:** the Definitive Edition as shipped, with only the mouse fixes: DE colours, HUD and radar at the game's size, intro and menu shown, the DE camera, the PC mouse camera fix and raw mouse input.
- **Recommended** (the defaults): PS2 colour adaptation with restored weather colours, HUD 0.8, radar 0.75, legal screens skipped, original camera behaviour.
- **Enhanced:** Recommended plus resuming the last save at startup.

Field of view is 1.0 in every preset. The save slot and the classic object LODs (a compatibility setting) aren't changed by presets.

## What the fix adds

- **Field of view** (0.5 to 2.0, default 1.0): multiplies the gameplay camera's field of view after the game's own widescreen widening; larger is wider. It is not applied while a cutscene runs, while the scripted widescreen borders are on, or in the sniper, rocket launcher, first-person weapon, camera, fixed and flyby views. Object visibility, sprites and third-person aiming use the same value; mouse and aim sensitivity are unchanged.
- **HUD scale** and **Radar scale**: scale the player information and radar widgets.
- **Startup and saves:** **Skip intro** skips the legal screens; **Resume on startup** (off by default) loads the last save once the main menu is ready; F5 quicksaves outside missions to the chosen **Save slot**.
- **Mouse and camera:** **Improve PC camera** (PC mouse camera fixes, aim sensitivity that follows the camera's field of view, a separate **Confine cursor** setting while the mouse camera is active), **Raw mouse input**, **Camera centering delay**, **Original camera** (original camera distances and controller orbit).
- **Colours:** **Visual style** (DE, PS2 or PC colours), **Restore weather colours**, **Colour filter strength**. The game's own tone mapping and HDR stay active; the original PS2 blur effect isn't reproduced.
- **Streaming:** **Classic object LODs** (on the Compatibility page) keeps the original lower-detail object models loaded.
- **Stability:** fixes running out of room for world objects when the whole map streams in, and vehicle engine sounds that could use up the game's sound slots at high frame rates or keep playing forever.

## Native to the remaster

Resolution and aspect ratio (any resolution, Hor+ widescreen), window modes, the frame limiter, anti-aliasing including DLSS, the menus and frontend, gamepad support with controller icons, and videos are the Definitive Edition's own and aren't touched by the fix. The fix adds no anti-aliasing of its own: the game's AA and DLSS stay in use, so the fix has no anti-aliasing setting.

## Known limits

- HUD aspect ratio constraints are not supported; the fix offers HUD and radar scaling.
- Cutscene field of view and cutscene borders are the game's own and aren't changed by the fix.
- No in-game settings menu; use fusionfix.io or FusionFixHub.

## Installation

Extract the release archive into the game directory, preserving its folders, then launch `SanAndreas.exe`.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
