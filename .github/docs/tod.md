<div align="center">

<img src="https://thirteenag.github.io/screens/tod/main2.jpg" width="760" alt="Total Overdose Fusion Fix">

**Total Overdose Fusion Fix** brings the console versions' widescreen HUD to the PC game, keeps the menus and the HUD in their proportions at any aspect ratio, and adds window modes, a field of view setting, video framing, a frame rate limit and faster startup.

[Website](https://thirteenag.github.io/wfp#tod) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TotalOverdose.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Any resolution, by default the desktop's; the resolution choices in the game's options use it too. The game's saved resolution is preserved for when the fix is not in use.
- **Aspect ratio and field of view** — Correct 3D projection at every aspect ratio from 4:3 to 32:9, with more view to the sides on wider screens.
- **HUD** — The consoles' widescreen HUD instead of the stretched 4:3 one: the radar, health and ammo sit at the screen's edges in their correct proportions, also on 21:9 and 32:9 screens. Messages and boxes in the middle stay centred, and banners across the screen span its whole width.
- **Menus** — The menus, the pause and objectives screens and the title keep their 4:3 proportions in the middle of the screen; the bars and backgrounds that cross the whole menu still fill the screen. The mouse points at what it is drawn over.
- **Raw mouse input** — A dedicated input thread collects mouse movement while the game retains its sensitivity, wheel and button behavior.
- **Display scaling** — The game gets the full resolution on scaled desktops.

## Options

- **Controller** — An Xbox-style (XInput) controller works without setup: the bundled Xidi turns it into the game's default keys and mouse. Left stick moves, right stick looks, RT fires, LT aims at the head, A jumps (and confirms in menus), B shootdodges (and skips tutorial texts), X reloads, Y uses vehicles and objects, LB/RB switch weapons, LS uses the Loco Move, RS honks, Back rewinds, Start pauses, the D-pad moves through menus and cycles Loco Moves (left/right) and forklift forks (up/down). Prompts keep showing keyboard keys.
- **HUD aspect ratio** — Auto (default) keeps the HUD at the screen's edges; a ratio such as 16:9 or 4:3 keeps it in a centred box of that shape, the left part at the box's left edge and the right part at its right edge. Applies at once.
- **Video mode** — How the story videos are shown: Fill (default) shows each video as large as the screen allows, Original shows it as the game does, inside its 4:3 frame. Both keep the video's own shape and show the whole picture. Applies at once.
- **Field of view factor** — A multiplier (0.5–2.0, default 1.0) on the gameplay camera, limited to 150 degrees across. Applies at once.
- **Skip intro** — Skips the SCi and Deadline logos and keeps the story intro. On by default. Needs a restart.
- **Skip title screen** — Goes directly to the main menu after the story intro. On by default. Needs a restart.
- **Window mode** — Fullscreen, Windowed (a centred window at the chosen resolution) or Borderless (a window covering the monitor). Needs a restart.
- **Frame rate limit** — 60 FPS by default. Choose 30–1000 FPS, the active monitor's refresh rate, or Unlimited. Applies at once.
- **Pause in background** — On by default. In a window mode, the game pauses while another window is active. Off keeps it running. Applies at once.
- **Confine cursor** — Keeps the mouse cursor inside the game window while the game is active.
- **Check for updates** — Off by default. When on, the fix checks fusionfix.io for a newer version and shows a notice in place of the version number at the top of the main menu. After the notice appears, FusionFixHub opens when you close the game if it is installed.
- **Resolution width / height** — Only in the cfg and FusionFixHub's text editor: 0 uses the desktop resolution.

---

## Settings and hub

Options › Plugins, in the main menu and in the pause menu, lists the fix and the other installed plugins on the game's own settings page. A plugin's page has its preset, a row per category (Game, Camera, Display) and Restore defaults; a category lists its settings with their values beside them. Settings marked * apply after a restart. The line under the list describes the selected setting. Up/Down select (holding repeats), Left/Right change the value, Enter opens a category, Escape goes back to the row it came from; Page Up/Page Down and Home/End jump through long lists, and the mouse wheel and the list's arrows scroll them. The mouse selects and clicks rows and the value arrows. A controller works the same way: d-pad or left stick, A, Start to go back, and the shoulder buttons for a page. F1 shows the whole help text when it is longer than the line under the list.

The fix creates `scripts/TotalOverdose.FusionFix.cfg` on first start. FusionFixHub can change live settings while the game runs, and fusionfix.io can export a config. Vanilla shows the startup logos and title screen, plays videos in the game's 4:3 frame and leaves the frame rate unlimited. Recommended and Enhanced skip the logos and title screen, fit videos to the screen and limit the game to 60 FPS. Auto is the default HUD aspect ratio and the value in all three presets exported by the site. Window mode and resolution remain player choices. Texts are available in English, French, German, Italian and Spanish.

## Known limits

- The title picture and the menu pictures are 4:3 art and stay 4:3 in the middle of the screen.
- The fix supports the unprotected `TOD.exe` of the digital release (GOG); the SecuROM-protected disc `TOD.exe` is not supported.
- Button prompts and the Controls menu show keyboard keys, also while you play with a controller; the controller follows the default key bindings, so rebinding keys moves controller buttons with them.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside `TOD.exe`, preserving the folders in the archive.
3. Launch the game once to create `scripts/TotalOverdose.FusionFix.cfg` with every setting.
4. Configure the fix in Options › Plugins, in FusionFixHub or on fusionfix.io. Restart when an option requires it.


<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark&layout=mobile"><source media="(max-width: 768px)" srcset="https://fusionfix.io/credits/supporters.svg?layout=mobile"><source media="(prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark"><img width="100%" src="https://fusionfix.io/credits/supporters.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
