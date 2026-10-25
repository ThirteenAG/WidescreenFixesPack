<div align="center">

<img src="https://thirteenag.github.io/screens/sh3/main2.jpg" width="760" alt="Silent Hill 3 Widescreen Fix">

**Silent Hill 3 Widescreen Fix** brings the game to any resolution and aspect ratio, with an optional modern over-the-shoulder camera, gamepad support, higher quality effects and its settings in the game's own Options screen.

[Website](https://thirteenag.github.io/wfp#sh3) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SilentHill3.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — The desktop resolution by default, or a resolution set in the fix's options; the 3D scene can render at its own resolution.
- **Widescreen** — Hor+ field of view. The interface, text, menus, items and map markers stay in a centred 4:3 area; effects placed from the 3D view (particles, blood, lens flares, haze, fog) and full-screen fades cover the whole screen. The mouse follows the 4:3 menus.
- **Cutscenes** — Never show what the PS2's cinematic bars covered: with the bars off the picture is enlarged to fill the screen.
- **Movies** — Keep their 14:9 shape or fill the screen; replacement movies keep their own shape.
- **Display** — No white window border. **Window mode** sets the startup display mode (fullscreen, windowed or borderless); Alt+Enter switches between fullscreen and the last window mode. Framed windows keep the chosen size on high-DPI displays, also when the window is as large as the desktop.
- **Stability** — No registry needed, no "safe mode" reset of the display settings after a crash, no long freeze each time Options opens, a steady 60 FPS, sound effects that went missing on some systems (#421), Father Vincent's glasses (#758).
- **Controllers** — Xidi with a modern layout, analog sticks without the stacked dead zones, pad button names in the button configuration, rumble.

## Options

- **In-game options** — The fourth Options page, **Plugins** (LB/RB, Page Up/Down or the title arrows), lists plugins with settings, with Widescreen Fix first. Open a plugin to change its settings; longer lists continue on numbered pages. Its **Preset** row selects Vanilla, Recommended or Enhanced; accept twice to confirm. **Restore Defaults** resets that plugin's settings. Most changes apply at once; settings that need a restart say so. Open this page from the title menu. Settings are saved in `scripts/SilentHill3.WidescreenFix.cfg` and can also be changed on fusionfix.io or in FusionFixHub.
- **Camera** — Classic fixed angles or a modern free camera over the shoulder (right stick or mouse), with its speed, inversion, distance and field of view.
- **Cinematic bars** — Black bars in cutscenes as on the PS2.
- **Movies and pictures** — Original framing or filling the screen, for the movies and for notes, pictures and the pause screen.
- **Effects** — Resolution of the soft shadows, the blur and depth of field and the pause background; finer fog; anisotropic filtering.
- **Intro** — Start at the title menu without the logos and the opening movie.

## Notes

The anti-aliasing option offers TAA, DLAA and FSR; it needs d3d8to9. DLAA and FSR need DXVK and the files from FusionFixHub's Anti-Aliasing page; they aren't part of the fix's archive. Anti-aliasing is off by default.

---

Button prompts follow the keyboard, mouse or controller you last used. A held button does not switch them back after you use another device.

**Confine cursor** keeps the pointer inside the game window. It can be changed while playing and is enabled by default in all window modes. The pointer is released when you leave the game, drag its window, open a system menu or turn the option off.

Long labels may be shortened with an ellipsis to leave room for their values.

## Known limits

- HUD width moves the health cross; other HUD elements and menus keep their centered 4:3 layout.
- Resolution changes require a restart. Raw mouse input and a configurable frame-rate limiter are not supported.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/SilentHill3.WidescreenFix.cfg`.
4. Optionally change the settings in the game's Options > Plugins, in FusionFixHub, or on fusionfix.io. Restart the game when an option requires it.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
