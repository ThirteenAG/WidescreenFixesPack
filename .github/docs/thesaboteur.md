<div align="center">

<img src="https://thirteenag.github.io/screens/thesaboteur/main2.jpg" width="760" alt="The Saboteur Fusion Fix">

**The Saboteur Fusion Fix** adjusts widescreen FOV, adds a configurable frame rate limit and window modes, and fixes interface and shutdown issues.

[Website](https://thirteenag.github.io/wfp#thesaboteur) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheSaboteur.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Field of view** — Adjust widescreen FOV, with scaling capped to avoid rendering problems at very wide ratios.
- **Interface and shutdown** — Correct map marker rendering and high-resolution cursor movement, and fix audio stuttering when closing the game.

## Options

- **Frame rate** — Set a frame rate limit, including values above 60 FPS.
- **Window mode** — Windowed or borderless when the game's fullscreen option is off.
- **SMAA and console gamma** — SMAA anti-aliasing before the HUD and the Xbox 360 gamma.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/TheSaboteur.FusionFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub, then restart the game. An old `scripts/TheSaboteur.FusionFix.ini` isn't used anymore and can be deleted.

## Settings

FusionFixHub and fusionfix.io change the settings; the first launch creates `scripts/TheSaboteur.FusionFix.cfg`.
The widescreen field-of-view correction changes live. Other settings apply after a restart. Presets: the widescreen field of view is on in all three; **Vanilla** has no
frame rate limit (the game's own timing); **Recommended** (the default) limits to 60 FPS with the fix's timing
corrections; **Enhanced** adds SMAA and the console gamma. The window mode is the player's choice.

## Known limits

- HUD aspect constraints, video framing, cinematic-border controls, intro skipping and raw mouse input are not supported.
- Anti-aliasing offers Off and SMAA through `GRAPHICS.AntiAliasing`; there is no sharpness control.

- No field of view multiplier: the widescreen correction applies to every camera, cutscenes included, and is capped
  at 2.1 times the 4:3 width (wider breaks the sky and objects).
- Above 60 FPS only driving, the clock and mouse look are corrected; other parts of the game may still run at the
  wrong speed.
- The fix doesn't force a window mode: **Window mode** applies when the game's own fullscreen option is off.
- No in-game settings menu.
