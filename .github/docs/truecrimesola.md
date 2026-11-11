<div align="center">

<img src="https://thirteenag.github.io/screens/truecrimesola/main2.jpg" width="760" alt="True Crime: Streets of LA Fusion Fix">

**True Crime: Streets of LA Fusion Fix** corrects resolution handling, aspect ratio and field of view for widescreen displays, keeps the menus and HUD in proportion, frames the story scenes, and adds the monitor's resolutions to the game's own options, a mouse camera, controller support, SMAA, console gamma, window modes and a frame rate limit.

[Website](https://thirteenag.github.io/wfp#truecrimesola) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TrueCrimeStreetsofLA.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Both versions** — Everything works on `TrueCrime_.exe` and on `TrueCrimeMB.exe`, the later build of the Eastern European editions.
- **Resolution** — Options › Display › Resolution lists your monitor's resolutions (up to 16, those of your desktop's shape first) and Apply Changes switches at once; the game keeps the choice in `TrueCrime.INI`. A first start uses the desktop resolution. The start-up crash with resolutions the game didn't know is fixed.
- **Aspect ratio and field of view** — Gameplay keeps the original vertical view and shows more at the sides on wider screens.
- **Menus** — The title, save, episode, options, pause, map and loading screens stay in their 4:3 frame in the middle of the screen with black sides.
- **HUD** — The HUD sits at the screen edges at any aspect ratio, with its original size.
- **Story scenes** — Scenes with the game's black bars keep their original framing on any screen.
- **Videos** — The startup videos keep their proportions.
- **Closing the window** — The game quits when its window is closed, as with Exit in the main menu.
- **High polling rate mice** — The game's message loop no longer stalls on thousands of mouse reports a second.
- **Display scaling** — The game gets the full resolution on scaled desktops.

## Options

- **Window mode** — Fullscreen, Windowed or Borderless, then restart. Windowed gives a centred window whose picture area is exactly the chosen resolution; Borderless always uses the desktop resolution.
- **Frame rate limit** — 60 by default; 0 is unlimited and -1 uses the monitor's refresh rate. Applies at once.
- **Anti-aliasing** — SMAA (default) smooths the edges of the 3D picture; the HUD and menus stay sharp. Applies at once.
- **Console gamma** — Off by default (on in Enhanced): the deeper contrast of the Xbox 360 gamma curve. Applies at once.
- **HUD aspect ratio** — Auto keeps the HUD at the screen edges; a ratio such as 16:9 or 4:3 keeps it in a centred box of that shape, the left part (portrait, health, radar) at the box's left edge and the right part at its right edge. Applies at once.
- **Cutscene borders** — Both (default) keeps the original framing: the game's bars above and below and, on screens wider than the original picture, bars at the sides. Letterbox keeps only the game's bars, Pillarbox keeps the picture at 4:3 between side bars, Off fills the screen and shows more at the sides. Applies at once.
- **Animated cutscene borders** — On by default: the bars slide in and out as in the original. Off shows them at once.
- **Movie framing** — Original shows the whole video inside the game's centred 4:3 frame; Fill fits the whole video to the screen. Nothing is cut off or stretched.
- **Field of view** — A multiplier (0.5–2.0, default 1.0) on the gameplay camera, limited to 150 degrees horizontally; applies at once. Story scenes keep their own.
- **Mouse camera** — The mouse turns the camera around Nick on foot (normal, fight and stealth mode) and around the car while driving. After the adjustable return delay (5 seconds by default) without mouse movement, or when the right stick moves the camera, the game's camera takes over again. Looking back from the car, story scenes and mission cameras always show the game's view and end the mouse view. Like the game's camera it moves in front of walls, cars and other obstacles and stays above the ground. In guns mode and in precision targeting (the slow-motion crosshair) the mouse turns the aim directly at the mouse camera speed, without the original's cut on fast moves, so a quick flick turns as far as a slow move; the keyboard and controller aim as in the original. The movement is read once per frame, so it stays smooth with high polling rates.
- **Invert mouse look** — Reverses vertical mouse movement for the added camera and aiming controls; applies at once and leaves controller input unchanged.
- **Mouse steering** — The game's own option, now off by default so the mouse turns the camera in cars; the fix keeps it in its settings instead of `TrueCrime.INI`, and the game's Controls menu still turns it on (the mouse then steers, as in the original). Changes apply while the game runs.
- **Mouse camera speed** — A multiplier (0.1–5.0, default 1.0) on how fast the mouse turns the camera. Applies at once.
- **Skip intro** — On by default: the game starts at the title screen, past the publisher and studio logo videos. Needs a restart.
- **Pause in background** — On by default: the game pauses while another window is active. Off keeps it running; controller input is ignored until the game is focused again.
- **Confine cursor** — Keeps the mouse cursor inside the game window while the game is active.
- **Check for updates** — Off by default. When on, the fix asks fusionfix.io once per start whether a newer version is out; FusionFixHub opens when you close the game.
- **Resolution width / height** — Only in the cfg and FusionFixHub's text editor: a fixed start resolution instead of the one chosen in the game. 0 uses the game's choice.

## Controller

The left and right stick dead zones can be adjusted independently in FusionFixHub or the settings file. Right-stick camera response and vertical inversion can also be adjusted; these changes apply live.

The PC version only reads keyboard and mouse during play. The fix adds any XInput controller (Xbox pads; PlayStation and Switch controllers through Steam Input or DS4Windows) to them, with no setup, and the keyboard and mouse keep working alongside:

| | On foot | In a car | Menus |
|---|---|---|---|
| Left stick | Move | Steer | Move the selection |
| Right stick | Camera | Camera | — |
| A | Jump | Handbrake | Confirm |
| B | Kick | Horn | Back |
| X | Punch | Siren | — |
| Y | Commandeer / leave car | Leave car | — |
| Right trigger | Fire / aim guns | Accelerate | — |
| Left trigger | Block | Brake / reverse | — |
| RB | Grab, frisk, pick up, throw | Fire | — |
| LB | Cover | Rear view | — |
| Left stick click | Reload | — | — |
| Right stick click | Centre camera | Car camera | — |
| D-pad | Up flash badge, down arrest, left fight mode, right gun mode | Up flash badge, right next radio track | Move the selection |
| Start | Pause menu, skip scenes | Pause menu | — |
| Back | City map | City map | — |

---

## Settings and hub

The fix creates `scripts/TrueCrimeStreetsofLA.FusionFix.cfg` on first start; FusionFixHub and fusionfix.io can change it, also while the game runs. Vanilla shows the intro, keeps the game's own cutscene bars, plays the logo videos with black bars, leaves the frame rate unlimited and turns SMAA off; Recommended and Enhanced skip the intro, add side bars in cutscenes, fit the logo videos to the screen, limit the frame rate to 60 and use SMAA; Enhanced adds console gamma. Window, resolution and mouse settings aren't changed by presets. Texts are available in English, French, German, Italian and Spanish.

## Known limits

- The game's menus are built from its own data files, so the fix has no page in them: change its settings in FusionFixHub or on fusionfix.io.
- Hints and prompts name keyboard keys also while a controller is used.
- The HUD keeps the original pixel size, so it is small at high resolutions, as in the original.
- The game's options list at most 16 resolutions.
- At 21:9 and wider, Letterbox alone shows the game's own picture without bars, cropped above and below, as the original does at that width.
- Exclusive fullscreen and switching away from it were not tested; use Borderless if Alt+Tab misbehaves.

## Installation

1. Download the fix archive from this release.
2. Extract it into the game folder, beside the game executable, keeping the folders in the archive. The loader is named `d3d8.dll` and needs its `d3d8.ini`.
3. Start the game once to create `scripts/TrueCrimeStreetsofLA.FusionFix.cfg` with every setting.
4. Configure the fix in FusionFixHub or on fusionfix.io. Restart when an option requires it.
5. If an older version of the fix installed `Xidi.32.dll`, `Xidi.ini` and `dinput8.dll`, they are no longer needed.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
