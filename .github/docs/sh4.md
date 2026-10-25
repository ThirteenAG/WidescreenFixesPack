<div align="center">

<img src="https://thirteenag.github.io/screens/sh4/main2.jpg" width="760" alt="Silent Hill 4: The Room Widescreen Fix">

**Silent Hill 4: The Room Widescreen Fix** brings the game to any resolution and aspect ratio, with an optional modern over-the-shoulder camera, analog gamepad controls, 30 FPS cutscenes and its settings in the game's own Options screen.

[Website](https://thirteenag.github.io/wfp#sh4) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SilentHill4.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — The desktop resolution by default, or a resolution set in the fix's options.
- **Widescreen** — Hor+ field of view. The interface, text, menus and pictures stay in a centred 4:3 area, while effects placed in the world, screen effects (blur, depth of field, glow, film, map zoom) and full-screen fades cover the whole screen; peep holes and the door scope get black sides (#556). The mouse hit areas follow the 4:3 menus (#554).
- **Cutscenes** — Run at 30 FPS instead of 15 and never show what the PS2's cinematic bars covered.
- **Display** — Exclusive full screen, a borderless window or a framed window. Alt+Enter switches between fullscreen and the last window mode; the gamma setting also works in a window, and the game no longer minimizes when it loses the focus.
- **Stability** — No registry needed, no "safe mode" reset to 640x480 after a crash, no failing hardware check (DirectSound effects and realtime shadows stay on), stereo sound that actually pans, and the French, German, Italian and Spanish text of the game can be chosen.
- **Controllers** — Analog movement and camera, a modern Xidi layout, pad button names in the button configuration, rumble, and mouse movement without slowdown.

## Notes

Modern anti-aliasing is not exposed in the settings.

## Options

- **In-game options** — The fourth Options page, **Option 4 (Plugins)** after Option 3 (the shoulder buttons or title arrows), lists plugins with settings, with Widescreen Fix first. Open a plugin to change its settings; longer lists continue on numbered pages. Its **Preset** row selects Vanilla, Recommended or Enhanced; accept twice to confirm. **Restore Defaults** resets that plugin's settings. Most changes apply at once; settings that need a restart say so. Open this page from the title or pause menu. Settings are saved in `scripts/SilentHill4.WidescreenFix.cfg` and can also be changed on fusionfix.io or in FusionFixHub.
- **Camera** — Classic or a modern free camera over the shoulder outside the apartment (right stick or mouse), with its speed, inversion, distance and field of view.
- **Cinematic bars** — Black bars in cutscenes as on the PS2.
- **Movies** — Original shape or filling the screen.
- **Intro** — Start at the title menu without the logos and the opening movie.

---

HUD Aspect Ratio Constraint applies live to the left-anchored health indicator: Auto uses the screen edge and a ratio uses a centered box. Inventory HUD elements and controller prompt icons are not covered. Gameplay FOV, four cutscene-border modes and the Original/Fill/Stretch movie choices are available.

Button prompts follow the keyboard, mouse or controller you last used. A held button does not switch them back after you use another device.

**Confine cursor** keeps the pointer inside the game window. It can be changed while playing and is enabled by default in all window modes. The pointer is released when you leave the game, drag its window, open a system menu or turn the option off.

## Known limits

- HUD width moves the health display; it does not reposition the inventory or every icon.
- Resolution changes require a restart. Raw mouse input and a configurable frame-rate limiter are not supported.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/SilentHill4.WidescreenFix.cfg`.
4. Optionally change the settings in the game's Options > Option 4 (Plugins), in FusionFixHub, or on fusionfix.io. Restart the game when an option requires it.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
