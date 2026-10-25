<div align="center">

<img src="https://thirteenag.github.io/screens/gta1/main2.jpg" width="760" alt="Grand Theft Auto Widescreen Fix">

**Grand Theft Auto Widescreen Fix** supports configurable resolution, camera and controls on fusionfix.io, in FusionFixHub or in the game's menu.

[Website](https://thirteenag.github.io/wfp#gta1) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTA1.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **In-game options** — The Options screen gains one Plugins item, with this fix first and Display, Camera, Controls and Game pages. They hold every setting of this fix, plus the language and keyboard controls of the original setup program. Changes are saved at once and most apply immediately, including the resolution. During gameplay, Esc or the controller's Back button pauses the game and opens a pause menu with Resume, one Plugins entry, and Quit Game. Quit Game opens the native quit prompt that Esc used to show. Language and classic keys are kept in the fix's settings file; the fix doesn't write the registry.
- **Resolution** — Set the width and height, or leave either at 0 to follow the desktop. The in-game resolution list (F11) offers modern modes. Resolutions up to 7680×4320 are supported.
- **HUD** — Text, portraits, weapon icons and messages scale independently of the world. **HUD safe area** adds a configurable inset, and **HUD aspect ratio** keeps the HUD in a centred box of an aspect ratio such as 16:9 (Auto: the whole screen); world arrows and floating labels keep their camera projection.
- **Frontend** — The original 640×480 menus are centered and scaled without stretching.
- **Windowing** — The default is the original exclusive fullscreen mode. The fix also offers frameless windows centred on the monitor, without changing the display mode or needing a DirectDraw wrapper. Display Mode in the options, or Alt+Enter, switches to a borderless window that covers the monitor with the picture scaled to fit. **Window mode** offers the original exclusive fullscreen, a window of the game's size (default) or a window covering the monitor. The window has no frame or title bar, and its corners are never rounded on Windows 11.
- **Frame pacing** — The simulation keeps its original speed with a precise timer.
- **Startup** — Missing language and control settings are supplied automatically. Game settings are read from the current user, then the machine installation; missing defaults are supplied in memory. The fix does not copy or write registry settings.

## Camera

- **Overhead camera** — Like GTA3's top-down camera, the view stays on the player while the game's look-ahead and height changes are smoothed in real time. **Field of view** widens the view (1.0, the original, by default); the extra zoom fades out as the game raises the camera with vehicle speed.
- **Rotating camera** — Like Chinatown Wars, the view turns so the player's vehicle faces the top of the screen, with arrows and labels turning with the world and the HUD staying upright. On foot the view keeps its last orientation by default. Movement controls follow the turned view. **Rotation smoothing** sets how quickly the view catches up; C toggles the rotation (the key can be changed; F6 is the game's own pause key).

## Controls

Modern controls are enabled by default and can be turned off in Controls. Gameplay bindings can be changed in Controls, on fusionfix.io or in FusionFixHub.

| Action | Keyboard/mouse | XInput controller |
|---|---|---|
| Move on foot / steer in vehicles | WASD | Left stick or D-pad |
| Aim while stationary | Hold a mouse button | Right stick |
| Fire | Left mouse button | X, or right trigger on foot |
| Enter/exit vehicle | F | Y |
| Jump/brake | Space | A |
| Special action | Shift | B |
| Previous/next weapon | Q / E | LB / RB |
| Accelerate/reverse | W / S | RT / LT |
| Pause | P | Start |

Menu navigation supports WASD, arrows, Enter and Escape, plus controller directions and A/B. Letter keys remain available in the native name editor. Modern gameplay input is restricted to single-player outside recording and replay.

Vehicle throttle uses RT/LT by default; the left stick and D-pad only steer. Enable **Stick Throttle** in Controls to use stick/D-pad up and down instead of the triggers. Keyboard W/S is available in either mode. In the native quit prompt, A confirms and B/Back cancels; gameplay actions are suppressed.

## Settings integration

Settings changed on fusionfix.io or in FusionFixHub apply while the game runs. In-game options, resolution changes and Alt+Enter are saved to the settings file. The Options screen has one Plugins row, listing this fix first and other plugins after it; each plugin has its own preset and Restore Defaults. Settings that can't be changed while paused are marked. Resume, P/F6 and the controller's Start button resume the game; plain F6 still shows the original Game Paused text. The menu text follows the game's language; choose another language in FusionFixHub.

There is no temporal anti-aliasing (TAA, DLAA or FSR): the game draws its picture in software.

The resolution picker includes reported display modes, including ultrawide modes, and keeps the configured size. Resolution and bindings are outside the built-in presets.

## Known limits

- The native font supports a limited Latin alphabet. Other characters use a question mark; punctuation is normalized.
- The original F11 mode list remains bounded by its native fixed records; the Display page provides the expanded picker.

## Compatibility

The fix targets Rockstar Classics GTA1 and London 1969/1961 (WINO), the original GTA1 retail executable and the original London 1969 and 1961 retail executables (unprotected or no-CD). Executables wrapped by copy protection (SafeDisc and similar) are not supported.

London’s chapter-select high-score panel still overlaps its header artwork.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game’s installation folder, preserving its directory structure. The included files belong under `WINO`.
3. Launch once to create `WINO/scripts/GTA1.WidescreenFix.cfg`. An old `WINO/scripts/GTA1.WidescreenFix.ini` isn't used anymore and can be deleted.
4. Configure on fusionfix.io, in FusionFixHub or in the game's menu. Changes from the hub or a preset apply at once, as from the in-game options, except the resolution, window mode, software buffer, frame pacing and the original setup values, which apply after a restart. Launch the game normally. A configuration application is not required for a fresh installation of the supported Windows builds.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
