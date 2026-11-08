<div align="center">

<img src="https://thirteenag.github.io/screens/sonicheroes/main2.jpg" width="760" alt="Sonic Heroes Fusion Fix">

**Sonic Heroes Fusion Fix** brings the game to any resolution and aspect ratio, with the HUD at the screen edges, a skippable intro and controller support.

[Website](https://thirteenag.github.io/wfp#sonicheroes) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SonicHeroes.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Any resolution; the desktop resolution by default.
- **Widescreen** — Hor+ field of view; the 3D view keeps its original height and widens at the sides, with object culling following the wider view.
- **HUD** — Score, time, rings and lives sit at the left screen edge, the team, Team Blast gauge and formation buttons at the right edge, with their original proportions. Menus, the pause menu, results and messages keep their centred 4:3 layout.
- **Lens flare, Extras windows, staff roll** — Sized and placed correctly at any aspect ratio.
- **Movies** — The game's movies are 4:3 pictures and are shown whole at their own shape, with black bars at the sides on wider screens. The story scenes are movies too, so there is no separate cutscene setting.
- **No disc needed** — The retail game runs without its disc.
- **Frame pacing** — Frames come at an even 60 per second, the game's own speed; movies keep the game's own timing.
- **Window icon** — The game's window and taskbar button show the game's icon.
- **Supported executables** — The retail `Tsonic_win.exe` and the modified `Tsonic_win2.exe`; the fix replaces the latter's own widescreen camera code.

## Options

- **Window mode** — Fullscreen, Windowed (optionally resizable) or Borderless (cover the monitor at its own resolution, fit the chosen aspect ratio, or the chosen resolution in a centred window). The window keeps its exact size on scaled (high-DPI) desktops.
- **HUD aspect ratio** — Auto keeps the HUD at the screen edges; 4:3, 16:9, 21:9 or 32:9 keep it in a centred box of that shape. Applies at once.
- **Field of view** — Multiplies the gameplay field of view (0.5 to 2.0, applies at once); the HUD is unchanged.
- **Skip intro** — Starts at the title screen without the SEGA logo and the title's entrance.
- **Skip Press Start** — Goes on from the title screen to the game data selection on its own.
- **Shadows** — Choose the original PC or GameCube-sized character shadows and a shadow-map resolution. The default map is 1024; the original uses 256.
- **Object distance** — Optionally shows objects from farther away, reducing pop-in.
- **Switch flash effect** — Original, or the PlayStation 2 versions of the castle/mansion switch flash.
- **Save folder** — Optionally keeps saves and the game's settings in a folder inside the game folder.
- **No frame skipping, ignore the mouse** — On by default. Attract demos can be restored, and the quit dialog, loading screen timeout and subtitles can be turned off.
- **Confine cursor** — Keeps the cursor inside the active game window; released on focus loss, while moving the window, in system menus and immediately when turned off.
- **Check for updates** — Off by default. When on, the game shows a notice at the title and main menu when a newer version of the fix is available.

Settings are on fusionfix.io or in FusionFixHub (English, French, German, Italian and Spanish), with the Vanilla, Recommended and Enhanced presets. HUD aspect ratio, field of view, confine cursor and update check apply at once, the rest at the next start. Resolution, minimum aspect ratio, clip range, save folder and object-distance tuning are on the advanced pages.

## Controller

The archive includes Xidi for the game's DirectInput controls. Connect an XInput controller, or expose a PlayStation or Nintendo controller through Steam Input or DS4Windows. The layout matches the button letters the game shows: A jumps and confirms, X is the action button and cancels, Y and B change formation, LB or RB starts the Team Blast, LT/RT and the right stick turn the camera, Start pauses, and the left stick or d-pad moves. The buttons can still be changed in the game's own Button Configuration.

## Known limits

- The game's options screens are fixed scenes from its data files, so there is no in-game Plugins page; the fix's settings are in FusionFixHub and on fusionfix.io.
- No Ctrl+F10 developer overlay: the game draws with Direct3D 8, which the overlay doesn't support.
- Two-player split screen keeps the game's own per-player HUD layout.
- Some menu decorations (the title's sliding background, the scrolling "SONIC HEROES" banner, the game data cards) reach past the 4:3 menu area on wider screens; nothing is stretched.
- The game keeps running in the background, as the original does.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/SonicHeroes.FusionFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.
