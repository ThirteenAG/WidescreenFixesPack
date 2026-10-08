<div align="center">

<img src="https://thirteenag.github.io/screens/gta2/main2.jpg" width="760" alt="Grand Theft Auto 2 Widescreen Fix">

**Grand Theft Auto 2 Widescreen Fix** corrects resolution handling and HUD scaling, adjusts the camera view, and adds a configurable quicksave hotkey.

[Website](https://thirteenag.github.io/wfp#gta2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTA2.WidescreenFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTA2.WidescreenFix/scripts/GTA2.WidescreenFix.ini)

</div>

---

## Fixes

- **In-game options** — The main menu's Options item opens an in-game page instead of the external GTA2 Manager. It holds every setting of this fix and those of the GTA2 Manager: display, graphics, sound, camera, modern and classic controls, language and game settings. Changes are saved at once and most apply immediately, including the resolution and volumes.
- **Resolution** — Set the output resolution with `ResX` and `ResY` in the INI file or on the in-game Display page, which lists every display mode, not only 4:3 ones.
- **Field of view** — Widescreen keeps the original vertical view and extends it sideways; the map is drawn out to the screen edges.
- **HUD** — Correct HUD scaling for widescreen.
- **Windowed mode** — `WindowedMode = 1` runs the game in a borderless window centred on the monitor, without changing the display mode. A window the size of the desktop covers the whole screen. Losing focus no longer minimizes it.
- **Startup** — Missing renderer and control settings are initialized, with game registry access redirected to the current user.
- **Frontend colours** — Menu images are decoded into the renderer's actual pixel format.
- **Movies** — Movie frames are fitted to the output without stretching, including the windowed Bink buffer path.
- **Target arrows** — Arrows next to a nearby target stay at the right distance when the camera is higher than default.
- **Exit** — With `SkipCredits`, quitting returns through the native frontend shutdown path, both from the Quit item and with Escape.

## Camera

- **Rotating camera** — Like Chinatown Wars, the view turns so the player's vehicle faces the top of the screen. On foot the view keeps its last orientation by default (`RotateOnFoot`). Movement controls follow the turned view, and the HUD stays upright. `RotationSmoothing` sets how quickly the view catches up; F6 toggles the rotation (`ToggleRotationKey`).
- **Smooth follow** — Like GTA3's top-down camera, the view stays on the player while the game's look-ahead and speed-dependent height are smoothed in real time (`SmoothFollow`, `FollowTime`, `HeightTime`).

## Options

- **Quicksave** — Save with a configurable hotkey.
- **Manual zoom** — Use the configured plus/minus keys.
- **Recovery** — F8 asks the native ground-height/teleport path to recover the player at the current horizontal position, without enabling the debug cheat system.
- **Frontend background** — `FillFrontendBackground = 0` shows the complete artwork with side bars. Set it to `1` to fill the screen by cropping the art proportionally.

## Controls

Modern single-player input is enabled by default: WASD movement, mouse aim while stationary, left mouse fire, F enter/exit, Space jump/brake, Shift special action, Q/E weapons and P pause. These gameplay keys are configurable in `[INPUT]` as Windows virtual-key codes; `0` disables a binding.

XInput maps movement to the left stick/D-pad, stationary aim to the right stick, fire to X (or RT on foot), enter/exit to Y, jump/brake to A, special action to B, weapons to LB/RB, vehicle acceleration/reverse to RT/LT, pause to Start and message replay to Back. Menus support controller navigation and WASD. Native text entry keeps letter keys.

Set `ModernControls = 0` and/or `Gamepad = 0` to use the native controls. Modern gameplay input is excluded from multiplayer and recording/replay.

## Notes

The fix targets GTA2 9.6. Multiplayer, replays and physical controller input still need broader validation.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Optionally edit `scripts/GTA2.WidescreenFix.ini` to configure the fix.
4. Launch the game.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
