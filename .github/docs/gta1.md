<div align="center">

<img src="https://thirteenag.github.io/screens/gta1/main2.jpg" width="760" alt="Grand Theft Auto Widescreen Fix">

**Grand Theft Auto Widescreen Fix** allows the output resolution to be configured through the INI file.

[Website](https://thirteenag.github.io/wfp#gta1) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTA1.WidescreenFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTA1.WidescreenFix/WINO/scripts/GTA1.WidescreenFix.ini)

</div>

---

## Fixes

- **In-game options** — The Options screen gains a More Options item with Display, Camera, Controls and Game pages. They hold every setting of this fix, plus the language and keyboard controls of the original setup program. Changes are saved at once and most apply immediately, including the resolution. During gameplay, Esc or the controller's Back button pauses the game and opens the same pages, with Resume and Quit Game. Quit Game opens the native quit prompt that Esc used to show. Language and classic keys are kept in the INI's `[SETUP]` section as well, because the game's per-user registry key is often read-only.
- **Resolution** — Set `ResX` and `ResY`, or use `0` to follow the desktop on either axis. The in-game resolution list (F11) offers modern modes. The software raster row table is expanded for resolutions up to 7680×4320.
- **HUD** — Text, portraits, weapon icons and messages scale independently of the world. `HudSafeArea` adds a configurable inset; world arrows and floating labels keep their camera projection.
- **Frontend** — The original 640×480 menus are centered and scaled without stretching.
- **Windowing** — The fix runs the game in a borderless window centred on the monitor, without changing the display mode and without a DirectDraw wrapper. Display Mode in the options, or Alt+Enter, switches to a borderless window that covers the monitor with the picture scaled to fit. `WindowedMode = 0` restores the original exclusive fullscreen.
- **Frame pacing** — The simulation keeps its original speed with a precise timer instead of the busy-waiting sound-system timer (`FixFramePacing`).
- **Startup** — Missing language and control settings are supplied automatically. Game registry access uses the current user, with existing machine settings copied on first use.

## Camera

- **Overhead camera** — Like GTA3's top-down camera, the view stays on the player while the game's look-ahead and height changes are smoothed in real time (`SmoothFollow`, `FollowTime`, `HeightTime`). `Zoom` widens the view (1.5 by default); the extra zoom fades out as the game raises the camera with vehicle speed.
- **Rotating camera** — Like Chinatown Wars, the view turns so the player's vehicle faces the top of the screen, with arrows and labels turning with the world and the HUD staying upright. On foot the view keeps its last orientation by default (`RotateOnFoot`). Movement controls follow the turned view. `RotationSmoothing` sets how quickly the view catches up; C toggles the rotation (`ToggleRotationKey`; F6 is the game's own pause key).

## Controls

Modern controls are enabled by default and can be disabled with `ModernControls = 0`. Gameplay bindings are configurable as Windows virtual-key codes in `[INPUT]`; `0` disables a binding.

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

## Compatibility

Every feature supports the Rockstar Classics GTA1 and London 1969/1961 executables (WINO), the original GTA1 retail executable and the original London 1969 and 1961 retail executables (unprotected or no-CD). Executables wrapped by copy protection (SafeDisc and similar) are not supported. Physical controller validation is still pending.

Windowed gameplay, the overhead and rotating cameras, walking and driving with modern controls have been checked at 1280×720 in GTA1 and London 1969. London’s chapter-select high-score panel still overlaps its header artwork. Longer mission sessions need further validation, and the original retail builds have been checked against their code but not yet played.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game’s installation folder, preserving its directory structure. The included files belong under `WINO`.
3. Optionally edit `WINO/scripts/GTA1.WidescreenFix.ini` to configure the fix.
4. Launch the game normally. A configuration application is not required for a fresh installation of the tested Windows builds.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
