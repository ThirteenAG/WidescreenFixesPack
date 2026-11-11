<div align="center">

<img src="https://thirteenag.github.io/screens/burnout3/main2.jpg" width="760" alt="Burnout 3: Takedown Fusion Fix (PCSX2F)">

**Burnout 3: Takedown Fusion Fix (PCSX2F)** makes the game widescreen without stretching: a wider view, the HUD at the screen edges, menus and videos in their own proportions, an adjustable field of view, 60 FPS and a startup video skip.

[Website](https://thirteenag.github.io/wfp#burnout3) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Burnout3.PCSX2F.FusionFix/main.cpp)

</div>

---

## Fixes

- **Aspect ratio** - follows the emulator's aspect ratio or window size, from 4:3 to 32:9; Display Aspect Ratio sets a ratio instead. The view widens on wide screens (the height of the picture stays as in the original) and nothing is stretched.
- **Field of view** - Field of View (0.5–2.0) widens or narrows the driving camera. Replays and crash cameras keep their own framing.
- **HUD** - position, lap, speed and boost meter sit at the screen edges in their original size. HUD Size changes their size. HUD Box (Auto, 4:3, 16:9, 21:9, 32:9 or a custom ratio) keeps them inside a centered area on wide screens; Auto uses the whole screen. Changing the emulator's aspect ratio or window size during a race moves the HUD at once.
- **Menus** - menus, dialogs, the Crash Nav map and loading screens keep their 4:3 proportions in the middle of the screen; the menu frame and its bands reach the screen edges.
- **Videos** - Original shows a video as the game does, inside its 4:3 picture. Fill shows the whole video as large as the screen allows at its own aspect ratio, with nothing cut off. Covers the startup videos, the menu background and the training videos.
- **60 FPS** - Frame Rate (`MAIN.FPSLimit` in the cfg) runs the game at 60 FPS with races, timers and videos at their normal speed; Original keeps the game's own rate.
- **Skip intro** - skips the legal notice, the Criterion logo and the opening video, straight to the Press START screen.
- **Camera look** - the right stick looks around while driving; the view returns when you release it. Gamepad Camera Speed and Invert Gamepad Y adjust it.

## Installation

1. Download the fix archive from this release.
2. Use [PCSX2F](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest) (PCSX2 Fork With Plugins; the fix does not work in regular PCSX2).
3. Extract the plugin archive into the emulator folder, preserving its `PLUGINS` directory.
4. Set the emulator's aspect ratio or window size, then launch the game.
5. If an older `PLUGINS/Burnout3.PCSX2F.WidescreenFix.elf` is still there, delete it. It is part of this Fusion Fix now; while it is installed, the Fusion Fix stays off and says so on screen.

## Useful information

- Compatible version: **SLUS-21050** (crc: **BEBF8793**).
- Keep PCSX2F's FMV aspect ratio switch off; the fix fits the videos itself.
- With the emulator's aspect ratio on Stretch, the fix follows the window's shape.

## Settings

Open **Plugins** in the main menu. The first page has Display Aspect, HUD Box, HUD Size, Field of View and Movies; **More** has Skip Intro, Preset, FPS and **Controls** (mouse and gamepad camera, side-by-side split screen). Left and right change a value. The same settings are in FusionFixHub, on fusionfix.io and in `PLUGINS/Burnout3.PCSX2F.FusionFix.cfg`, which the fix creates on the first start. Display settings apply during play within about a second and after loading a save state; Skip Intro and Frame Rate apply the next time the game starts, and the game says so when you change them.

- **Presets** - Vanilla shows the startup videos, plays videos as the game does and keeps the original frame rate; Recommended (the default) and Enhanced skip the startup videos, use Fill for videos and 60 FPS. Aspect ratio, HUD, field of view and controls keep your choice.
- **Side-by-side split screen** - places the two players' views side by side in the next split-screen race instead of above each other.
- **Mouse camera** - Mouse Sensitivity and Invert Mouse Y adjust looking around with the mouse while driving.

## Known limits

- The Press START screen stays; press Start to continue.
- After changing the emulator's aspect ratio or window size while the game runs, the d-pad icon in the menu button bar keeps the old width until the game is restarted. The HUD, menus and videos follow at once.
- Burnout 3 has no story cutscenes, so there are no cutscene border options.
- Button prompts show the PlayStation 2 buttons.
- Resolution, anti-aliasing and window modes are handled by the emulator.

## Presets and FusionFixHub

- Setting texts: English, French, German, Italian and Spanish.
