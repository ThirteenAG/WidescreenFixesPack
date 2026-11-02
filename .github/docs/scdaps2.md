<div align="center">

<img src="https://thirteenag.github.io/screens/scdaps2/main2.jpg" width="760" alt="Splinter Cell: Double Agent Fusion Fix (PCSX2F)">

**Splinter Cell: Double Agent Fusion Fix (PCSX2F)** corrects the widescreen aspect ratio, field of view, HUD, menus, cutscenes, videos and loading screens.

[Website](https://thirteenag.github.io/wfp#scdaps2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellDoubleAgent.PCSX2F.FusionFix/main.cpp)

</div>

---

## Fixes

- **Aspect ratio** - follows the emulator's aspect ratio or window size, at any width; Display Aspect Ratio (FusionFixHub) sets a ratio instead. The view widens on wide screens without stretching.
- **Field of view** - Field of View (0.5–2.0) widens or narrows the gameplay camera. Cutscenes, scripted scenes, aiming, first-person and gadget views keep their own framing.
- **HUD** - the stealth meter, weapon panel, communication messages and the rest of the gameplay HUD sit at the screen edges in their original proportions. HUD Scale changes their size. HUD Aspect Ratio (Auto, 4:3, 16:9, 21:9, 32:9 or a custom ratio) keeps the HUD inside a centered area on wide screens; Auto uses the whole screen. In co-op each player's HUD is placed within that player's half.
- **Menus** - the launcher screens (memory card notices, title, Press START, Game Mode) and the game's own menus keep their 4:3 proportions in the middle of the screen.
- **Single-player cutscenes** - keep the original framing at any aspect ratio. Cutscene Borders chooses the bars: Off (the cutscene fills the screen), Letterbox (the original bars above and below), Pillarbox (the original picture without the top and bottom bars, with bars at the sides) or Both (the original 4:3 picture with its bars, centered). The bars slide in and out like the original unless Animate Cutscene Borders is off.
- **Videos** - Original shows a video as the game does, inside its 4:3 picture. Fill shows the whole video as large as the screen allows at its own aspect ratio. Covers the startup logos, the teaser, the menu background and the story videos.
- **Loading screens** - the loading artwork and its progress bar keep their proportions; loading runs without the emulator's frame limiter.
- **Skip intro** - skips the logo movies at startup. Memory card notices and the title screen stay.
- **Single-player start** - fixes the launcher's switch to the single-player game, which otherwise fails in PCSX2 Fork.

## Installation

1. Download the fix archive from this release.
2. Use [PCSX2 Fork With Plugins](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest).
3. Extract the plugin archive into the emulator folder, preserving its `PLUGINS` directory.
4. Set the emulator's aspect ratio or window size, then launch a supported version of the game.
5. If an older `PLUGINS/SplinterCellDoubleAgent.PCSX2F.WidescreenFix.elf` is still there, delete it. It is part of this Fusion Fix now; while it is installed, the Fusion Fix stays off and says so on screen.

## Useful information

- Two versions of the game are compatible, **SLUS-21356** (crc: **C0498D24**) and **SLES-53827** (crc: **ABE2FDE9**).
- Keep PCSX2's FMV aspect ratio switch off; the fix fits the videos itself.

![](https://i.imgur.com/swnrFGs.jpg)

## Settings

The settings are stored in `PLUGINS/SplinterCellDoubleAgent.PCSX2F.FusionFix.cfg`, which the fix creates on the first start. Change them in FusionFixHub or on fusionfix.io. The game's menus are built from its data files, so the fix adds no settings page inside the game. Display settings apply during play within about a second, and after loading a save state; Skip Intro applies the next time the emulated game starts.

- **Presets** - Vanilla shows the startup logos, original letterbox bars and the original video composition; Recommended (the default) and Enhanced skip the logos, use Both for cutscene borders and fit the whole video. Field of view, HUD, display aspect ratio and bar animation keep your choice.
- **Text language** - Choose the language of unsupported-game notices; Auto uses English.

## Known limits

- Co-op cutscenes keep the game's own presentation.
- Button prompts show the PlayStation 2 buttons.
- The game runs at its original frame rate.
- Resolution, anti-aliasing and window modes are handled by the emulator.

## Presets and FusionFixHub

- Setting texts: English, French, German, Italian and Spanish.
