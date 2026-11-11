<div align="center">

<img src="https://thirteenag.github.io/screens/re6/main2.jpg" width="760" alt="Resident Evil 6 Fusion Fix">

**Resident Evil 6 Fusion Fix** adds ultrawide support with the HUD at the screen's edges, fills the screen with local co-op views, makes the variable frame rate cap adjustable, and adds startup, window and controller options with a settings page in the game's own Options menu.

[Website](https://thirteenag.github.io/wfp#re6) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ResidentEvil6.FusionFix/)

</div>

---

## Fixes

- **Ultrawide HUD** — The HUD keeps its shape at any aspect: the health and ammo gauges, the objective text and the other side elements go to the screen's edges; full-screen overlays (low health, partner waiting, the low health frame) cover the whole screen.
- **Menus** — Menus, records, loading screens and the pause menu stay in a centred 16:9 frame on wider screens.
- **Objective markers** — The 3D objective markers point at their targets at any resolution, also in split screen.
- **Local co-op** — Each player gets half of the screen (top/bottom, or left/right).
- **Cutscenes** — Keep their framing at any aspect; subtitles stay inside the picture.
- **Film grain** — Keeps its size at high resolutions.
- **Camera** — No clipping into walls next to the camera at wide aspects.
- **Display modes** — No startup crash on monitors with 256 or more display modes; the resolution list keeps all of them, including widths that aren't a multiple of eight.
- **Frame rate** — The **VARIABLE** frame rate option is capped at the chosen limit instead of 120; the 30 and 60 FPS options stay as they are.
- **Windows settings** — The game no longer turns off the sticky keys, toggle keys and filter keys shortcuts or changes mouse trails and the screen saver setting of Windows; the display stays on while the game runs.

## Options

The fix's settings are in the game's **Options › Plugins** page (F1 or Y shows the full help of a setting), in FusionFixHub and on fusionfix.io.

- **HUD aspect ratio** — Auto (default) puts the HUD at the screen's edges; a ratio such as 16:9 or 21:9 keeps it in a centred box of that shape. Applies at once.
- **Field of view** — Multiplier for the normal and vehicle gameplay cameras, also under Options › Camera; scripted, weapon and cinematic cameras keep their framing.
- **Cutscene borders** — Off, Letterbox (the original bars), Pillarbox (16:9 with side bars on wider screens) or Both (default). Applies at once.
- **Video framing** — Original shows the videos inside the game's centred 16:9 frame. On screens wider than 16:9, Fill (default) enlarges the two story videos, which have black bars in the file, until their picture fits the screen. No picture is cut off or stretched.
- **Frame rate limit** — What the game's VARIABLE frame rate option is capped at: 120 FPS (default), Unlimited, the refresh rate or a number. Applies at once.
- **Window mode** — Windowed or borderless when the game's own Full Screen option is off. A window as big as the screen has its frame outside the screen.
- **Pause in background** — On by default; off keeps the game running while its window isn't active.
- **Confine cursor** — Keeps the mouse cursor inside the game window while the game is active.
- **Skip intro** — Skips the Capcom and engine logos and the startup notices that only need OK ("Load successful." and the auto-save notice); the game starts at the main menu. Needs a restart.
- **Skip first title prompt** — Continues past the first Press Start prompt.
- **Controller icons** — Xbox (Auto), PlayStation or Nintendo button icons.
- **Split screen** — Vertical (left/right) layout and swapped views.
- **Low health / dying effects, objective marker** — Can be hidden.
- **RGB lighting** — Support for [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).
- **Check for updates** — Off by default. When on, the fix asks fusionfix.io once per start whether a newer version is out and tells you in the Plugins page.

---

## Settings and hub

The fix creates `scripts/ResidentEvil6.FusionFix.cfg` on first start; FusionFixHub and fusionfix.io can change it, also while the game runs. Vanilla keeps the original cutscene bars, the intro and the videos in the game's 16:9 frame; Recommended and Enhanced skip the intro, fit the story videos to the screen and use both cutscene borders. Window and input settings aren't changed by presets. Texts are available in English, French, German, Italian and Spanish.

## Known limits

- The game's resolution list and full screen option are its own; the fix only adds the window modes for its windowed mode.
- Controller icons replace the game's Xbox button art; a PlayStation or Nintendo pad is read as an Xbox pad (through Steam Input or DS4Windows).
- The display lists support at most 8192 modes, 256 distinct resolutions and 32 refresh rates.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/ResidentEvil6.FusionFix.cfg`.
4. Configure the fix in Options › Plugins, in FusionFixHub or on fusionfix.io. Restart the game when an option requires it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
