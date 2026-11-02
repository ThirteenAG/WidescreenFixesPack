<div align="center">

<img src="https://thirteenag.github.io/screens/sh2/main2.jpg" width="760" alt="Silent Hill 2 Fusion Fix">

**Silent Hill 2 Fusion Fix** corrects widescreen scaling and field of view, with options for cutscene borders, frame rate and camera speed, plus lighting and compatibility repairs.

[Website](https://thirteenag.github.io/wfp#sh2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SilentHill2.FusionFix/dllmain.cpp)

</div>

---

## This project is a part of [Silent Hill 2: Enhanced Edition](https://enhanced.townofsilenthill.com/SH2/). It is recommended to use it instead.

The Enhanced Edition includes broader game repairs. This standalone fix keeps the original Direct3D 8 renderer and adds the options below.

## Display and camera

- **Resolution and window mode** — Choose a custom resolution, a framed window, fullscreen, or borderless. Zero resolution uses the desktop size. Borderless renders at the monitor's size. Resolution and window mode require a restart.
- **Widescreen interface** — Corrects the aspect of menus, inventory, text and background images. The interface stays centered; there is no separate HUD-width control.
- **Field of view** — Changes the normal gameplay camera live. 1.0 preserves the Fusion Fix's normal view; widening stops at 150 degrees across. Scripted cameras keep their own framing.
- **Cutscene borders** — On preserves the original picture between the game's bars at every aspect ratio. Off removes the bars and uses the wider camera view. On is the default.
- **Videos** — Movies keep their encoded aspect. Original shows them as the game does, whole inside its centred 4:3 frame with black bars around them; Fill fits the whole movie to the screen. Nothing is cut off or stretched.

## Settings

Open **Options → Plugins → Fusion Fix** from the title or pause menu. Up/Down scroll the settings, Left/Right change values, Enter selects, and Escape goes back. F1 opens the selected option's full help; Up/Down scroll long help. A single notice marks changes that require a restart. The page also offers Vanilla, Recommended and Enhanced presets and Restore Defaults. Presets preserve resolution, window mode, language and controller preferences.

Settings are saved in `scripts/SilentHill2.FusionFix.cfg`. FusionFixHub and fusionfix.io use the same settings and help in English, French, German, Italian and Spanish. The old fix INI is unused.

**Pause on Focus Loss** pauses gameplay while another window is active. **Confine Cursor** restricts the mouse to the active game window. **Skip Intro** skips startup logos, leaving story movies intact. **Check for Updates** is off by default; when enabled, a notice appears in the native Options menu.

## Controllers and compatibility

The archive includes Xidi for the game's native DirectInput controller path. The default mapper supplies movement, right-stick camera and native button actions while preserving keyboard bindings. Controller Icons offers Auto (Xbox), Xbox, PlayStation and Nintendo artwork for the native controller binding names. The native Controls page still manages assignments. F1 help also opens with the controller's map button (Y in the supplied layout).

The default frame limit is 30 FPS. The limit changes live; 0 is unlimited and -1 follows the monitor refresh rate. Higher rates can still affect animations, scripts and camera behavior. The PS2 camera-speed option is intended for 30 FPS. Legacy Single Core Affinity is optional and off by default; enable it only for affected audio-looping systems.

The fix also repairs a flashlight bug and the fog and lighting of one ending cutscene, lets the same button be assigned to several actions, stops the game from resetting its settings after it closes, creates the local.fix file that anti-aliasing needs, and prevents a crash with Steam's controller configuration. Noise resolution, fast transitions and full-screen background images remain options with their own help.

## Known limits

- The native resolution picker retains its six original slots; use the fix's resolution settings for custom sizes. Window-mode changes require a restart.
- This fix does not repair every high-frame-rate or audio issue. Keep 30 FPS for the original timing.
- Controller artwork covers native binding-name text; hardcoded instructions and every physical controller/rumble combination are not guaranteed.
- No developer overlay is included for the native Direct3D 8 renderer.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/SilentHill2.FusionFix.cfg`.
4. Open Options → Plugins, or change the settings on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
