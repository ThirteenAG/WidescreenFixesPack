<div align="center">

<img src="http://thirteenag.github.io/screens/truecrimenyc/main2.jpg" width="760" alt="True Crime: New York City Fusion Fix">

**True Crime: New York City Fusion Fix** makes the PC version work at any resolution and aspect ratio: a wider view instead of a cut-off one, the HUD at the screen's sides, menus and movies at their own shape, cinematic borders that keep the original framing, window modes, a mouse camera, gamepad support and 60 FPS with the correct game speed. A separate fix for the PlayStation 2 version is described below.

[Website](http://thirteenag.github.io/wfp#truecrimenyc) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TrueCrimeNewYorkCity.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Any resolution** — The game's startup settings dialog (shown on the first start, or with Ctrl held) lists every display mode. In a window any size works.
- **Field of view** — The view widens with the screen (Hor+) instead of being cut off at the top and bottom.
- **HUD** — The health silhouette, badge meter, weapon and money stay at the right side of the screen, the radar, crime alerts, button prompts and phone at the left side, the street sign and objectives in the middle; nothing is stretched.
- **Menus, map and loading screens** — Kept at their 4:3 shape in the middle of the screen, with the map markers in place.
- **Cutscene and title scene borders** — The cinematic borders are sized for the screen's height instead of a fixed size, and keep the original picture at any aspect ratio.
- **Movies** — The startup movies are shown whole at their own shape, never stretched or cut off.
- **Window and focus** — Correct window size at any Windows display scaling, square window corners, no stretched picture from the window frame. Switching to another window pauses the game (or keeps it running, see below) without minimizing it.
- **Gamepad** — Xbox and other XInput controllers work through the included Xidi with the game's own gamepad layout, in the game and in the menus (A select, B back, Start exit).
- **Tree foliage** — The leaves of the trees and bushes in Central Park and other parks are drawn again, lit by the time of day like on the PlayStation 2; the PC version showed bare trees.
- **See-through leaves** — Potted bushes, flags, signs and other cut-out textures get soft, see-through edges like on the PlayStation 2 instead of dark outlines.
- **Traffic** — Cars no longer appear at the roadside or far ahead only to vanish a moment later. Traffic stays at the sides of a wide picture instead of disappearing there, and cars and pedestrians fade in and out rather than popping.
- **Install path** — The game loads its data from the Data folder next to the exe, so it runs from any folder.

## Options

- **HUD Aspect Ratio** — Auto spreads the HUD across the whole screen. A ratio such as 16:9 or 21:9 keeps it in a centered box of that shape; elements keep their place relative to the box.
- **Field of View** — 0.5 to 2.0 times the corrected view for walking and driving. Aiming, cutscenes, scripted views, loading and pause keep their own framing.
- **Cutscene Borders** — Both (default) keeps the original cinematic picture with borders above and below or at the sides. Letterbox keeps only the original borders above and below and shows more at the sides. Pillarbox limits the picture to the 4:3 frame without the borders above and below. Off fills the screen. **Animated Cutscene Borders** lets them slide in and out like the original.
- **Video Aspect Ratio** — Fill shows the startup movies as large as the screen allows at their own shape; Original shows them in the 4:3 frame the game used.
- **Window Mode** — Fullscreen, a window at the resolution chosen in the game's settings dialog, or borderless at the monitor's resolution. The window size is fixed while the game runs. Needs a restart.
- **Pause When Unfocused** — Pauses the game while another window is active; off keeps it running.
- **Confine Cursor** — Keeps the cursor inside the game window while it is active.
- **Skip Intro** — Skips the logo movies and the legal notice at startup.
- **Frame Rate** — 30 (original) or 60 FPS with the correct game speed; scenes that need it still run at 30. It can be changed while playing.
- **Language** — The game's language (English US/UK, French, German, Spanish, Italian, Russian) when its files are installed.
- **Mouse camera** — The mouse turns the camera on foot and in vehicles and moves the map cursor, with its own speed and vertical inversion on top of the game's settings; both apply while playing. It is read every drawn frame, so it stays smooth with high polling rate mice. After 5 seconds without mouse movement the game's camera takes over again; Look Behind always shows the game's view behind and then returns to the game's camera.
- **High Resolution Shadows**, **Distance Blur** (like the PlayStation 2 version), **Bloom**, **Anti-Aliasing** (SMAA) and **Console Gamma** (Xbox 360 contrast).
- **Check for Updates** — Once per start, asks fusionfix.io whether a newer version of the fix is out; with FusionFixHub installed, it opens on the update when you close the game.

Presets: Recommended is the default. Vanilla keeps the original look (original cutscene borders, intro movies, 30 FPS, no added effects) with all fixes applied; Enhanced adds console gamma.

Change the settings in FusionFixHub, on fusionfix.io or in `scripts/TrueCrimeNewYorkCity.FusionFix.cfg`. Supporters can also open the settings overlay with Ctrl+F10. The game's own options are fixed menu pages, so the fix adds no page there.

## Known limits

- The window can't be resized while the game runs; choose its size in the game's settings dialog.
- HUD elements of situations not shown above (for example some mission timers and meters) may still sit in the 4:3 area.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game. The fix creates `scripts/TrueCrimeNewYorkCity.FusionFix.cfg` on its first start.

The included ASI loader (`d3d8.dll` with `d3d8.ini`) also provides the Direct3D 8 to 9 conversion used by the effects; keep both beside the game executable. `dinput8.dll`, `Xidi.32.dll` and `Xidi.ini` are the gamepad support.

Credits:

- [@Parallellines0451](https://github.com/Parallellines0451) (for Xbox 360 gamma)
- [@Tervel1337](https://github.com/Tervel1337) (for bloom and depth of field)
- [@ICantReadYourMind](https://github.com/ICantReadYourMind) (for PostFX module)

## **PCSX2F: True Crime New York City Fusion Fix**

Compatible versions:

- **SLUS-21106** (crc: **42A9C4EC**).
- **SLES-53616** (crc: **7A9D67B8**).
- **SLES-53617** (crc: **77B4F13C**).
- **SLES-53618** (crc: **B1AC3BEB**).

Installation:

- Download and extract [PCSX2F](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest) (PCSX2 Fork With Plugins; the fix does not work in regular PCSX2).
- Download and extract plugin archive to the root directory, where exe is located, keeping its `PLUGINS` folder.
- Change the aspect ratio or window size of the emulator.
- Launch the game.
- Use a BIOS of the same region as the disc. The US disc stops with "Jump to unaligned address" after the logo video on a European BIOS, with or without the fix.

### Fixes

- **Aspect ratio** — follows the emulator's aspect ratio or window size, at any width; Display Aspect Ratio sets a ratio instead. The view widens on wide screens without stretching.
- **Field of view** — Field of View (0.5–2.0) widens or narrows the gameplay camera on foot and in vehicles. Cutscenes keep their own framing.
- **HUD** — health, badge, weapon and ammunition, the phone and the help boxes sit at the screen edges in their original proportions. HUD Aspect Ratio (Auto, 4:3, 16:9, 21:9, 32:9 or a custom ratio) keeps them inside a centered area on wide screens; Auto uses the whole screen. Subtitles and centered messages stay centered.
- **Menus** — the memory card notice, the copyright screen, the title screen, main menu, name entry, save and load screens, pause menu and loading screens keep their 4:3 proportions in the middle of the screen; the 3D backgrounds fill the screen.
- **Logo video** — the Activision and Luxoflux video is shown at its 4:3 shape with black bars at the sides, never cut off.
- **Cutscenes** — keep the original framing at any aspect ratio. Cutscene Borders chooses the bars: Off (the cutscene fills the screen), Letterbox (the original bars above and below), Pillarbox (the original 4:3 picture at full height, with bars at the sides) or Both (the original 4:3 picture with its bars, centered). The bars slide in and out like the original unless Animated Cutscene Borders is off.
- **60 FPS** — the game runs at 60 frames per second; the one mission that needs the original speed keeps 30.
- **Faster loading** — the emulator runs at full speed during startup and loading screens.
- **Skip intro** — skips the logo video at startup. The memory card notice and the title screen stay.

### Settings

The settings are stored in `PLUGINS/TrueCrimeNewYorkCity.PCSX2F.FusionFix.cfg`, which the fix creates on the first start. Change them in FusionFixHub or on fusionfix.io; the texts are in English, French, German, Italian and Spanish. The game's menus are built from its data files, so the fix adds no settings page inside the game. Display settings apply during play within about a second, and after loading a save state; Skip Intro, 60 FPS and Faster Loading apply the next time the emulated game starts.

- **Presets** — Vanilla shows the logo video, keeps the original letterbox bars, 30 FPS and normal loading speed; Recommended (the default) and Enhanced skip the logo video, use Both for cutscene borders, 60 FPS and faster loading. Field of view, HUD, display aspect ratio and bar animation keep your choice.

### Known limits

- If the emulator window changes shape during play, the 3D view and the HUD size follow at once; the HUD elements already on screen return to their places after the pause menu is opened and closed or after the next loading screen.
- On screens narrower than 4:3 the HUD and menus are fitted to the screen width.
- Button prompts show the PlayStation 2 buttons.
- Resolution, anti-aliasing and window modes are handled by the emulator.

![truecrimenycpcsx2f](https://github.com/user-attachments/assets/5a34aa37-e4a4-4ceb-89c8-f3a0d49a205e)

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
