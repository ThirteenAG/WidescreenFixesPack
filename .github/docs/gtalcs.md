<div align="center">

<img src="https://thirteenag.github.io/screens/gtalcs/main2.jpg" width="760" alt="Grand Theft Auto: Liberty City Stories Widescreen Fix (PCSX2F)">

**Grand Theft Auto: Liberty City Stories Widescreen Fix (PCSX2F)** corrects widescreen aspect ratio, field of view, HUD and frontend layout. FMVs remain uncorrected.

[Website](https://thirteenag.github.io/wfp#gtalcs) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTALCS.PCSX2F.WidescreenFix/main.cpp)

</div>

---

## Fixes

- **Aspect ratio** - follows the emulator's aspect ratio or window size, at any width.
- **Field of view** - Widens the gameplay view without stretching; cutscenes keep their 16:9 framing. Gameplay FOV (0.5–2.0) adjusts the view further without changing cutscenes.
- **HUD** - Keeps HUD elements in proportion at the screen edges. HUD Scale changes their size. HUD Aspect Ratio Constraint (Auto, 16:9, 21:9 or a custom ratio) keeps the HUD inside a centered area on wide screens; Auto uses the whole screen.
- **Frontend and map** - kept at 4:3 proportions; full-screen artwork is contained with black sides.
- **Cutscene borders** - Choose Off, Letterbox (top and bottom bars on screens narrower than 16:9), Pillarbox (side bars on wider screens) or Both. The bars slide in and out.
- **Free-aim reticle** - stays round at any aspect ratio.
- **Skip intro** - skips the startup intros.
- **Options** - 60 FPS (off by default), fast loading (unthrottled emulator while loading), blur on/off, keyboard and mouse controls with mouse camera and rebindable actions.

The vehicle-light add-on keeps the original artwork in its Vanilla preset. Recommended and Enhanced enable the replacement lights. Changing the add-on setting requires restarting the emulated game.

## Installation

1. Download the fix archive from this release.
2. Use [PCSX2 Fork With Plugins](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest).
3. Extract the plugin archive into the emulator folder, preserving its `PLUGINS` directory.
4. Set the emulator’s aspect ratio or window size, then launch a supported version of the game.

## Useful information

One version of the game is compatible, **SLUS-21423** (crc: **7EA439F5**).

## Settings

The settings are stored in `PLUGINS/GTALCS.PCSX2F.WidescreenFix.cfg`, which the fix creates on the first start. Change them in FusionFixHub, on fusionfix.io, or in the game: the Display menu has one **Plugins** entry that lists this fix first, then Improved Vehicle Lights when it is installed. The fix's page has its preset (Vanilla, Recommended, Enhanced), frame rate, HUD size, cutscene borders, blur, fast loading, keyboard and mouse controls with bindings, and Restore Defaults. Menu changes apply at once and are saved when the menu closes; changes made on fusionfix.io or in FusionFixHub apply the next time the emulated game starts. Field of view, HUD aspect ratio and skip intro are set on fusionfix.io or in FusionFixHub only.

- **Presets** - Vanilla keeps the original look (cutscene letterbox, blur, intros, normal loading); Recommended is the default, and Enhanced is the same. Presets never change controls, mouse sensitivity or bindings.

After updating from an older release, reapply your preferences in FusionFixHub.

## Not covered

- **Resolution, anti-aliasing, window modes and Alt+Tab** - handled by the emulator.
- **Videos** - not aspect-corrected; Skip intro skips the startup videos.
- **Controller icons** - the game keeps its PS2 button art; with keyboard and mouse controls on, prompts show PC keys.

## Improved vehicle lights settings

Vehicle Lights is on by default. Change it on the Improved Vehicle Lights page under Plugins or in FusionFixHub, then restart the emulated game.

## Presets and FusionFixHub

- FusionFixHub text: English, French, German, Italian and Spanish.
- The added in-game settings menu is in English only.
