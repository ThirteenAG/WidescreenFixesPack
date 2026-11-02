<div align="center">

<img src="https://thirteenag.github.io/screens/kr/main2.jpg" width="760" alt="Knight Rider: The Game Fusion Fix">

**Knight Rider: The Game Fusion Fix** corrects resolution handling, aspect ratio, HUD scaling and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#kr) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/KnightRider.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution and aspect ratio** — Supports custom resolutions, including render targets wider than the original 2048-pixel limit. The 3D view widens at the sides while keeping its original height, and visibility follows the corrected camera.
- **HUD** — Side elements follow the chosen HUD box while centered elements stay centered. The normal HUD and cockpit retain their proportions. The cockpit keeps its original centered 4:3 view with black side areas on wider displays; its embedded instruments remain aligned with that view.
- **Frame rate** — Replaces both native frame caps with one adjustable limiter. Game time follows real time at higher frame rates.
- **Window modes** — Fullscreen, framed windowed and borderless modes, with a DPI-aware client area. Resolution and mode changes require restarting: scene cameras retain the DirectDraw renderer, and its native reset rejects outstanding references. The game has no renderer reset for dragging the window edges.
- **Intro skip** — Skips the startup logos and legal screen. Story scenes remain available.
- **Start-up abort** — Fixes the start-up abort on systems where the game's mouse setup fails.

## Options

- **Field of view factor** — 0.5–2.0, live; 1.0 preserves the intended gameplay view. Scripted shots use their own framing.
- **Resolution width / height** — 0 selects the desktop resolution; restart required.
- **Window mode** — Fullscreen, Windowed or Borderless; restart required. Borderless uses the desktop resolution.
- **HUD aspect ratio constraint** — Auto uses the screen edges; 4:3, 16:9, 21:9 or 32:9 limits the HUD to a centered box. Applies live.
- **Frame rate limit** — 0 is unlimited, −1 follows the display refresh rate; a positive value selects the cap. Default 60; applies live.
- **Cutscene borders** — Off or Pillarbox; applies live. Pillarbox keeps the original story framing with side borders.
- **Pause when unfocused** — Pauses gameplay while another window is active; applies live.
- **Confine cursor** — Keeps the cursor inside the focused game window; applies live.
- **Skip intro** — Enabled by default; restart required.
- **Skip press start** — Automatically continues past the startup Press Start screen; restart required.
- **Controller icons** — Auto, Xbox, PlayStation or Nintendo button labels; applies live. Auto uses Xbox labels because XInput does not identify the controller family.
- **Text language** — Auto, English, French, German, Italian or Spanish for the fix's settings and help.
- **Check for updates** — Optional update notice at the main menu; disabled by default.

Settings can be changed on fusionfix.io or in FusionFixHub. Live settings apply while the game runs; other changes show a restart requirement. Recommended is the default preset. Vanilla and Enhanced are also available; no preset changes the resolution.

## Controller

The archive includes Xidi for the game's native DirectInput controls. Connect an XInput controller, or expose a PlayStation or Nintendo controller through Steam Input or DS4Windows.

Use the left stick to steer, RT to accelerate and LT to brake. A activates Turbo Boost, B reverses, X activates Super Pursuit Mode, Y scans, LB/RB ski left/right, the right-stick button toggles night vision and the left-stick button resets KITT. View/Back changes camera, Menu/Start pauses or starts a mission, and d-pad left/right selects the scan target. Menus use the d-pad, A to confirm and B to return. The Controls page switches its button labels with the last device used.

The PC engine has fixed camera positions and no free-look camera or force-feedback effects. No separate camera-speed, invert-look or vibration option is needed. The game uses rendered story scenes and contains no separate video movies requiring a playback-mode option.

## Installation

1. Extract the fix archive beside the game executable, preserving its folders and the included Xidi files.
2. Start the game once to create `scripts/KnightRider.FusionFix.cfg`.
3. Change settings in FusionFixHub or on fusionfix.io.

The original options pages use fixed scene assets and have no extensible Plugins page. The fix's settings are available through FusionFixHub. Supporters can also press Ctrl+F10 for the developer overlay.

## PCSX2F: Knight Rider Fusion Fix

Compatible version: **SLES-51011** (crc **989192FE**).

### Fixes

- **Aspect ratio** — follows the emulator's aspect ratio or window size, at any width; Display Aspect Ratio sets a ratio instead. The 3D view widens at the sides and keeps its original height, without stretching.
- **Field of view** — Field of View (0.5–2.0) widens or narrows the driving camera, live. The scenes at the start of a mission keep their own framing.
- **HUD** — the destination panel with its truck view sits at the left edge, the minimap and the speed and damage display at the right edge, the direction arrow in the middle, all in their original proportions. HUD Aspect Ratio (Auto, 4:3, 16:9, 21:9, 32:9 or a custom ratio) keeps the panels at the edges of a centered box; Auto uses the whole screen. Full-screen effects such as night vision cover the whole screen.
- **Cockpit view** — the dashboard is a 4:3 model, so the cockpit view shows the original 4:3 picture with black sides on wider screens. Its HUD stays on the dashboard.
- **Menus and loading screens** — keep their 4:3 proportions in the middle of the screen; the background bands fill the width.
- **Mission start scenes** — Cutscene Borders chooses the bars: Off (the scene fills the screen), Letterbox (the original bars above and below), Pillarbox (the original 4:3 framing with bars at the sides) or Both. Subtitles stay in the 4:3 picture with Pillarbox and Both, and move to the left edge with Letterbox and Off.
- **Videos** — the videos are widescreen pictures inside a 4:3 frame. Original shows them as the game does; Fill shows the whole widescreen picture as large as the screen allows, with bars only where the screen is wider or narrower than the picture.
- **Skip intro** — skips the Davilex logo and the opening video at startup. The language choice, copyright and memory card screens stay.

### Installation

1. Use [PCSX2 Fork With Plugins](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest).
2. Extract the plugin archive into the emulator folder, preserving its `PLUGINS` directory.
3. Set the emulator's aspect ratio (Stretch follows the window) or window size, then launch the game. Keep PCSX2's FMV aspect ratio switch off; the fix fits the videos itself. Resolution and anti-aliasing are chosen in PCSX2.

### Settings

The settings are stored in `PLUGINS/KnightRider.PCSX2F.FusionFix.cfg`, which is created on the first start. Change them in FusionFixHub or on fusionfix.io. The game's menus are fixed scene files, so the fix adds no settings page inside the game. Display settings apply during play within about a second and after loading a save state; Skip Intro applies the next time the emulated game starts.

- **Presets** — Vanilla shows the intro, the original letterbox bars and the videos as the game shows them; Recommended (the default) and Enhanced skip the intro, use Both for the borders and Fill for the videos. Field of view, HUD aspect ratio and display aspect ratio keep your choice.
- Setting texts: English, French, German, Italian and Spanish.

### Known limits

- Button prompts show the PlayStation 2 buttons.
- The game runs at its original frame rate.
- Window modes, resolution and anti-aliasing are handled by the emulator.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
