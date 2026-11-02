<div align="center">

<img src="https://thirteenag.github.io/screens/sh3/main2.jpg" width="760" alt="Silent Hill 3 Fusion Fix">

**Silent Hill 3 Fusion Fix** brings the game to any resolution and aspect ratio, with an optional modern over-the-shoulder camera, gamepad support, higher quality effects and its settings in the game's own Options screen.

[Website](https://thirteenag.github.io/wfp#sh3) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SilentHill3.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — The desktop resolution by default, or a resolution set in the fix's options; the 3D scene can render at its own resolution.
- **Widescreen** — Hor+ field of view. The low-health cross sits in the screen's corner (or in the corner of the chosen HUD box); text, menus, items and map markers stay in a centred 4:3 area; effects placed from the 3D view (particles, blood, lens flares, haze, fog) and full-screen fades cover the whole screen. The mouse follows the 4:3 menus.
- **Cutscenes** — With Both the original shot is kept exactly at any aspect ratio: the game's black bars above and below, and bars at the sides on wider screens. Off removes all bars and fills the screen Hor+.
- **Movies** — Shown whole at their 14:9 shape (replacement movies at their own): Original inside the game's centred 4:3 frame, Fill fitted to the screen.
- **Display** — No white window border. **Window mode** sets the startup display mode (fullscreen, windowed or borderless); Alt+Enter switches between fullscreen and the last window mode. Framed windows keep the chosen size on high-DPI displays, also when the window is as large as the desktop.
- **Stability** — No registry needed, no "safe mode" reset of the display settings after a crash, no long freeze each time Options opens, a steady 60 FPS, sound effects that went missing on some systems (#421), Father Vincent's glasses (#758).
- **Controllers** — Xidi with a modern layout, analog sticks without the stacked dead zones, controller-binding artwork and native rumble.

## Options

- **In-game options** — The fourth Options page, **Plugins** (LB/RB, Page Up/Down or the title arrows), lists plugins with settings, with Fusion Fix first. Open a plugin to change its settings; longer lists scroll within the same plugin page. Its **Preset** row selects Vanilla, Recommended or Enhanced; accept twice to confirm. **Restore Defaults** resets that plugin's settings. F1 or the native Map button opens the selected option's complete help; Up/Down scroll long help and Escape goes back. Most changes apply at once; one footer notice marks changes that require a restart. Open this page from the title menu. Settings are saved in `scripts/SilentHill3.FusionFix.cfg` and can also be changed on fusionfix.io or in FusionFixHub. Camera mode remains the player's choice when changing presets.
- **Field of view** — The gameplay multiplier changes live, with 1.0 preserving the normal view and widening bounded to 150 degrees across. Scripted cameras and aiming keep their own framing.
- **Camera** — Classic fixed angles or a modern free camera over the shoulder (right stick or mouse), with its speed, inversion, distance and field of view.
- **Cutscene borders** — Letterbox keeps the game's bars above and below, Pillarbox keeps the original 4:3 picture with bars at the sides, Both keeps the original framing exactly, Off uses the full screen Hor+. Changes apply at once, also during a cutscene.
- **Movies and pictures** — Movies are shown whole in both choices; notes, pictures and the pause screen keep their original framing or fill the screen.
- **Effects** — Resolution of the soft shadows, the blur and depth of field and the pause background; finer fog; anisotropic filtering.
- **Intro** — Start at the title menu without the logos, the content warning and the opening movie (the opening movie still plays when the title menu is left idle).

## Notes

Controller Icons offers Auto/Xbox, PlayStation and Nintendo. Auto uses Xbox; other families are chosen manually. Keyboard and mouse binding names remain unchanged. The archive uses Xidi's built-in XInput backend; other pads can use Steam Input or DS4Windows.

**Pause in background** pauses gameplay while the window is inactive. Turn it off to keep the game running. **Check for Updates** is off by default; enabling it shows available updates in the native Options screen.

**Confine cursor** keeps the pointer inside the game window. It can be changed while playing and is enabled by default in all window modes. The pointer is released when you leave the game, drag its window, open a system menu or turn the option off.

Long labels may be shortened with an ellipsis to leave room for their values.

## Known limits

- The game's only corner HUD element is the low-health cross; messages, menus and the inventory keep their centred 4:3 layout.
- The native resolution picker retains its original slots. Set custom display and render sizes in the fix's CFG, FusionFixHub or fusionfix.io, then restart.
- The native 60 FPS cap remains. The modern camera uses native DirectInput mouse counts.
- Artwork covers native controller-binding names; hardcoded instructions retain the game's text.
- No Ctrl+F10 developer overlay: the game draws with Direct3D 8, which the overlay doesn't support. Every setting is in the game's Options > Plugins page, FusionFixHub and fusionfix.io.
- The window size is set at startup; resizing the window by dragging isn't supported.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/SilentHill3.FusionFix.cfg`.
4. Optionally change the settings in the game's Options > Plugins, in FusionFixHub, or on fusionfix.io. Restart the game when an option requires it.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
