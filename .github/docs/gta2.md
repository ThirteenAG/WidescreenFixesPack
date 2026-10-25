<div align="center">

<img src="https://thirteenag.github.io/screens/gta2/main2.jpg" width="760" alt="Grand Theft Auto 2 Widescreen Fix">

**Grand Theft Auto 2 Widescreen Fix** corrects resolution handling and HUD scaling, adjusts the camera view, and adds a configurable quicksave hotkey.

[Website](https://thirteenag.github.io/wfp#gta2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTA2.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **In-game options** — The main menu's Options item opens an in-game page instead of the external GTA2 Manager. It holds every setting of this fix and those of the GTA2 Manager: display, graphics, sound, camera, modern and classic controls, language and game settings. Changes are saved at once and most apply immediately, including the resolution and volumes.
- **Resolution** — Set the output resolution on fusionfix.io, in FusionFixHub or on the in-game Display page, which lists every display mode, not only 4:3 ones.
- **Field of view** — Widescreen keeps the original vertical view and extends it sideways; the map is drawn out to the screen edges.
- **HUD** — Correct HUD scaling for widescreen. **HUD aspect ratio** keeps the HUD in a centred box of an aspect ratio such as 16:9, its elements at the box's sides; 4:3 keeps the original layout in the middle, Auto uses the whole screen.
- **Window mode** — **Window mode** Borderless (the default) runs the game in a borderless window centred on the monitor, without changing the display mode. A window the size of the desktop covers the whole screen. Losing focus no longer minimizes it.
- **Startup** — Missing renderer and control settings are filled in from the installed values or the defaults. Options are saved to the fix's settings file; the fix doesn't write the registry.
- **Frontend colours** — Menu artwork keeps its expected colours.
- **Movies** — Movie frames keep their aspect ratio, including in windowed mode: **Videos** shows them at their original aspect ratio with black bars, filling the screen with the edges cut (default), or stretched. **Skip intro** skips the intro movie.
- **Target arrows** — Arrows next to a nearby target stay at the right distance when the camera is higher than default.
- **Exit** — With **Skip credits**, quitting skips the credits, both from the Quit item and with Escape.

## Camera

- **Rotating camera** — Like Chinatown Wars, the view turns so the player's vehicle faces the top of the screen. On foot the view keeps its last orientation by default. Movement controls follow the turned view, and the HUD stays upright. **Rotation smoothing** sets how quickly the view catches up; F6 toggles the rotation (the key can be changed).
- **Smooth follow** — Like GTA3's top-down camera, the view stays on the player while the game's look-ahead and speed-dependent height are smoothed in real time.

## Options

- **Quicksave** — Save with a configurable hotkey.
- **Manual zoom** — Use the configured plus/minus keys.
- **Recovery** — F8 returns a stuck player to ground level at the current position.
- **Frontend background** — Shows the complete artwork with side bars by default, or fills the screen by cropping the art.

## Controls

Modern single-player input is enabled by default: WASD movement, mouse aim while stationary, left mouse fire, F enter/exit, Space jump/brake, Shift special action, Q/E weapons and P pause. These gameplay keys can be changed in Controls, on fusionfix.io or in FusionFixHub.

XInput maps movement to the left stick/D-pad, stationary aim to the right stick, fire to X (or RT on foot), enter/exit to Y, jump/brake to A, special action to B, weapons to LB/RB, vehicle acceleration/reverse to RT/LT, pause to Start and message replay to right-stick click. Back opens the native quit prompt, A confirms and B/Back cancels. Gameplay actions are suppressed in that prompt. Menus support controller navigation and WASD. Native text entry keeps letter keys.

Vehicle stick/D-pad up and down do not accelerate or brake by default. Enable **Stick Throttle** in Controls to use them instead of RT/LT. Keyboard W/S is available in either mode. Score pop-ups follow the rotated world while keeping their digits facing the screen.

Turn off **Modern controls** and/or **Gamepad** to use the native controls. Modern gameplay input is excluded from multiplayer and recording/replay.

## Settings and hub

The settings also include the GTA2 Manager's options. An old `scripts/GTA2.WidescreenFix.ini` isn't used anymore and can be deleted. Game-language choices are limited to installed text files. Changes from FusionFixHub or a preset apply at once where the in-game options apply them at once, the others after a restart.

There is no temporal anti-aliasing (TAA, DLAA or FSR). Advanced Options keeps the original setup pages and adds one **Plugins** entry. Plugins lists this fix first, followed by other plugins with settings. Each plugin has its own preset, remaining settings and **Restore Defaults**, applied with confirmation. Settings already on a setup page stay there. Long lists and help continue on additional pages; a notice identifies changes that need a restart. Choose the menu language in FusionFixHub; Automatic follows the game.

Pause with **P/Start**, then press **Enter/A** on **Advanced Options**. Arrows, D-pad or stick navigate; Left/Right change a value and Enter/A confirms. Back/Escape returns to the pause screen; P/Start resumes the game. Setup options that require the main menu are disabled while paused.

## Known limits

- Unsupported font characters use a visible question mark; the native alphabet does not cover arbitrary Unicode.
- No framed window mode and no gamepad icons.
- Resolution and bindings stay outside built-in presets. The Display picker includes reported ultrawide modes.

## Notes

The fix targets GTA2 9.6.

**Field of view** changes the gameplay view range from 0.5 to 2.0; larger values show more of the world.

The default window mode is the original exclusive fullscreen mode.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/GTA2.WidescreenFix.cfg`.
4. Configure the fix on fusionfix.io, in FusionFixHub or in the game's menu. Hub changes apply at once where the in-game options apply them at once, the others after a restart.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
