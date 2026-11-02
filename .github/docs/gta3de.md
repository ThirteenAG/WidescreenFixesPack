<div align="center">

<img src="https://thirteenag.github.io/screens/gta3de/main2.jpg" width="760" alt="Grand Theft Auto III: The Definitive Edition Fusion Fix">

**Grand Theft Auto III: The Definitive Edition Fusion Fix** adds a gameplay field-of-view multiplier, HUD aspect-ratio constraints and HUD/radar scaling, F5 quicksaves, startup skips, raw mouse input and mouse camera adjustments, with an option to disable first-person rifle aiming, and restores the PS2 and PC colour looks.

[Website](https://thirteenag.github.io/wfp#gta3de) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTA3DE.FusionFix/dllmain.cpp)

</div>

---

## Configuration

Configure on fusionfix.io or in FusionFixHub. The first start creates `Gameface/Binaries/Win64/scripts/GTA3DE.FusionFix.cfg`. Changes apply while the game runs, except Skip intro, Resume on startup, the save slot and Classic object LODs, which apply at the next start.

Presets:
- **Vanilla:** the Definitive Edition as shipped, with widescreen HUD and mouse corrections: DE colours, HUD and radar at the game's size, intro and menu shown, the DE camera, the PC mouse camera fix and raw mouse input.
- **Recommended** (the defaults): PS2 colour adaptation with restored weather colours, HUD 0.8, radar 0.75, legal screens skipped, original camera behaviour, third-person rifle aim.
- **Enhanced:** Recommended plus Resume on startup (loads your last save straight from startup).

Field of view is 1.0 in every preset. The save slot and the classic object LODs (a compatibility setting) aren't changed by presets.

## What the fix adds

- **Field of view** (0.5 to 2.0, default 1.0): multiplies the gameplay camera's field of view after the game's own widescreen widening; larger is wider. It is not applied while a cutscene runs, while the scripted widescreen borders are on, or in the sniper, rocket launcher, first-person weapon, camera, fixed and flyby views. Object visibility, sprites and third-person aiming use the same value; mouse and aim sensitivity are unchanged.
- **HUD Aspect Ratio**: Auto places side-anchored HUD widgets at the screen edges, with sizes based on screen height. Choose 4:3, 16:9, 21:9, 32:9 or enter another ratio to place the HUD in a centered box. The box, HUD scale and radar scale apply live and follow window resizing.
- **HUD scale** and **Radar scale**: scale the player information and radar widgets while keeping their edge anchors.
- **Startup and saves:** **Skip intro** skips the legal screens; F5 quicksaves outside missions to the chosen **Save slot**.
- **Mouse and camera:** PC mouse camera fixes and aim sensitivity that follows the camera's field of view are always active, with a separate **Confine cursor** setting while the mouse camera is active, **Raw mouse input**, **Camera centering delay**, **Original camera** (original camera distances and controller orbit), **Third-person rifle aim** (third-person aiming for rifles).
- **Colours:** **Visual style** (DE, PS2 or PC colours), **Restore weather colours**, **Colour filter strength**. The game's own tone mapping and HDR stay active; the original PS2 blur effect isn't reproduced.
- **Streaming:** **Classic object LODs** (on the Compatibility page) keeps the original lower-detail object models loaded.
- **Check for updates** (off by default): once per start, asks fusionfix.io whether a newer version of the fix is out and tells you in the game's help box; with FusionFixHub in the game folder, the hub opens on the fix's page when you close the game. Off makes no network request.
- **Unsupported game versions:** a game version the fix doesn't know is reported in a message box, and the game starts without the fix.

## Native to the remaster

Resolution and aspect ratio (any resolution, Hor+ widescreen), window modes, the frame rate options, pausing when the window loses focus, anti-aliasing including DLSS, the menus and frontend (kept in a 16:9 box on wider screens), gamepad support with controller icons and its control schemes, and videos are the Definitive Edition's own and aren't touched by the fix. The fix adds no anti-aliasing of its own: the game's AA and DLSS stay in use, so the fix has no anti-aliasing setting.

## Known limits

- Cutscene field of view and cutscene borders are the game's own and aren't changed by the fix.
- No in-game settings menu; use fusionfix.io or FusionFixHub.

## Installation

Extract the release archive into the game directory, preserving its folders, then launch `LibertyCity.exe`.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
