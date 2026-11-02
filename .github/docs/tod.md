<div align="center">

<img src="https://thirteenag.github.io/screens/tod/main2.jpg" width="760" alt="Total Overdose Fusion Fix">

**Total Overdose Fusion Fix** brings the console versions' widescreen HUD to the PC game, keeps the menus and the HUD in their proportions at any aspect ratio, and adds window modes, a field of view setting, video framing, a frame rate limit, controller support through Xidi and faster startup.

[Website](https://thirteenag.github.io/wfp#tod) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TotalOverdose.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Any resolution, by default the desktop's; the resolution choices in the game's options use it too. The game's saved resolution is preserved for when the fix is not in use.
- **Aspect ratio and field of view** — Correct 3D projection at every aspect ratio from 4:3 to 32:9, with more view to the sides on wider screens.
- **HUD** — The consoles' widescreen HUD instead of the stretched 4:3 one: the radar, health and ammo sit at the screen's edges in their correct proportions, also on 21:9 and 32:9 screens. Messages and boxes in the middle stay centred, and banners across the screen span its whole width.
- **Menus** — The menus, the pause and objectives screens and the title keep their 4:3 proportions in the middle of the screen; the bars and backgrounds that cross the whole menu still fill the screen. The mouse points at what it is drawn over.
- **Display scaling** — The game gets the full resolution on scaled desktops.

## Options

- **Controller** — The game reads gamepads through DirectInput; the fix comes with Xidi, which presents an Xbox-style (XInput) controller to the game as a standard gamepad.
- **HUD aspect ratio** — Auto (default) keeps the HUD at the screen's edges; a ratio such as 16:9 or 4:3 keeps it in a centred box of that shape, the left part at the box's left edge and the right part at its right edge. Applies at once.
- **Video mode** — How the story videos are shown: Fill (default) shows each video as large as the screen allows, Original shows it as the game does, inside its 4:3 frame. Both keep the video's own shape and show the whole picture. Applies at once.
- **Field of view factor** — A multiplier (0.5–2.0, default 1.0) on the gameplay camera, limited to 150 degrees across. Applies at once.
- **Skip intro** — On by default: the game skips the SCi and Deadline logos. The story intro and title screen remain. Needs a restart.
- **Window mode** — Fullscreen, Windowed (a centred window at the chosen resolution) or Borderless (a window covering the monitor). Needs a restart.
- **Frame rate limit** — 60 FPS by default; also the monitor's refresh rate, unlimited, or a number of frames per second. Applies at once.
- **Pause in background** — On by default: in a window mode the game pauses while another window is active, as it always has. Off keeps it running.
- **Confine cursor** — Keeps the mouse cursor inside the game window while the game is active.
- **Check for updates** — Off by default. When on, the fix asks fusionfix.io once per start whether a newer version is out and shows it in place of the version number at the top of the main menu; FusionFixHub opens when you close the game.
- **Resolution width / height** — Only in the cfg and FusionFixHub's text editor: 0 uses the desktop resolution.

Ctrl+F10 opens the developer overlay for supporters.

---

## Settings and hub

The fix creates `scripts/TotalOverdose.FusionFix.cfg` on first start; FusionFixHub and fusionfix.io can change it, also while the game runs. Vanilla shows the intro, plays the videos in the game's 4:3 frame and leaves the frame rate unlimited; Recommended and Enhanced skip the logos, show the videos as large as the screen allows and limit the game to 60 FPS. Window, resolution and other player choices aren't changed by presets. Texts are available in English, French, German, Italian and Spanish.

**Raw mouse input** uses relative movement from Windows while keeping the game’s button, wheel and sensitivity handling. It applies immediately.

## Known limits

- The game's menus are compiled into the game as scripts, so the fix has no page in them: change its settings in FusionFixHub or on fusionfix.io.
- The title picture and the menu pictures are 4:3 art and stay 4:3 in the middle of the screen.
- Raw mouse movement and high-polling-rate behavior need broader gameplay testing.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside `TOD.exe`, preserving the folders in the archive.
3. Launch the game once to create `scripts/TotalOverdose.FusionFix.cfg` with every setting.
4. Configure the fix in FusionFixHub or on fusionfix.io. Restart when an option requires it.


<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
