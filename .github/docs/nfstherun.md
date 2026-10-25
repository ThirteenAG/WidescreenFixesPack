<div align="center">

<img src="https://thirteenag.github.io/screens/nfstherun/attract.jpg" width="760" alt="Need for Speed: The Run - FusionFix">

**Need for Speed: The Run Fusion Fix** unlocks the frame rate, including cutscenes, fixes crashes, improves the windowed mode, adds a free-look camera, extra graphics options, unlock options and quality-of-life improvements.

[Website](https://thirteenag.github.io/wfp#nfstherun) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSTheRun.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Crash Fix** - fixes the random crashes during races, where the renderer met an object whose mesh streaming had already freed
- **Minimap Fix** - fixes parts of the road missing on the minimap
- **Challenge Series** - Escape gets back out of the Challenge Series list when playing offline, instead of starting to sign in
- **Display scaling** - the game gets the full resolution on scaled desktops, and its window has square corners on Windows 11
- **High frame rates** - snow spray, drift smoke and dirt dust no longer fill the screen above 30 FPS, and the engine sound no longer gets stuck at one tone

---

## New Options

<img src="https://thirteenag.github.io/screens/nfstherun/introskip.jpg" width="360" align="right" style="margin: 0 0 0 24px;" alt="Gameplay">

- **Skip Intro** - enables faster loading by bypassing startup FMVs and login prompts; without the Origin client running the game no longer waits for it at startup
- **Field of View** - multiplies the race camera's field of view (0.5 to 2.0, default 1.0, applies at once); cutscenes and scripted shots keep their own
- **Improved Windowed Mode** - makes the game's windowed mode borderless and resizes the window to match the selected resolution
- **Unlock Frame Rate** - the game uses the VSync setting from its options everywhere, so it isn't held at 30 FPS in load screens and parts of races (game version 1.1)
- **Unlock Cutscenes** - cutscenes and quick-time events also run above 30 FPS, with the characters moved smoothly on every frame and clean camera cuts
- **FPS Limit** - frame rate limit in races, applies at once; 0 keeps the game's own (turn VSync off in the game's settings to go above the refresh rate)
- **Smooth Reflections** - the car reflections move with the car on every frame instead of 30 times a second
- **Console Gamma** - a custom gamma curve that creates a more contrasted image with deeper colors, similar to how GTA IV looked on the Xbox 360; off by default, on in the Enhanced preset
- **Anisotropic Filtering** - minimum anisotropic filtering level (16 by default), from the next level loaded
- **Shadow Resolution / Distance** - higher shadow map resolution and a longer shadow draw distance than the game offers
<br clear="both">

---

## Camera

<img src="https://thirteenag.github.io/screens/nfstherun/camera.gif" width="400" align="right" style="margin: 0 0 16px 24px;" alt="Mouse look in-game">

- **Mouse/Stick Look** - free look for the chase camera with the right stick or the mouse. Like the car camera of GTA San Andreas, the look direction stays fixed in the world while the car turns; after the return delay the camera eases back behind the car. Experience *Need for Speed: The Run* like never before with new camera angles. Return delay, return speed, stick and mouse sensitivity, invert look and horizontal-only right stick are configurable on fusionfix.io or in FusionFixHub.
- Press **LS + RS** simultaneously to toggle between the original Free Look camera and the new one.
<br clear="both">

---

## Unlocks

- **Unlock All Cars** - unlocks all vehicles in the garage
- **Unlock Challenges** - unlocks all challenge content
- **Unlock Everything** - unlocks all content in the game

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `Need For Speed The Run.exe`.
3. Launch the game once to create `plugins/NFSTheRun.FusionFix.cfg`, then configure options on fusionfix.io or in FusionFixHub.
4. Launch the game.

---

## Widescreen

- **Aspect ratio and field of view** - the game's own: the 3D view takes its aspect ratio from the viewport and keeps the vertical field of view (Hor+). The fix adds no aspect code; Field of View multiplies the race camera on top.
- **HUD and menus** - every Scaleform screen is shown with "show all" scaling, centered, so the 16:9 HUD and menus stay in a centered 16:9 box at wider aspect ratios and fit the width at narrower ones; nothing is stretched. There is no HUD aspect setting, as the screens can't be laid out at another aspect without changing them.
- **Cutscenes** - in-engine, with the same Hor+ view, so they show more at the sides on wider screens. The game draws no cinematic letterbox bars, and the fix has no cutscene border setting.
- **Videos** - VP6 movies are drawn into a 1280x720 texture that the Scaleform screens show, so they keep 16:9 like the menus; there is no separate full-screen movie path to change. Skip Intro skips the startup movies.
- **Window modes** - the game's own Fullscreen/Windowed option, with Improved Windowed Mode turning the windowed mode borderless, gives fullscreen, windowed and borderless; there is no separate window mode setting.
- **Settings** - on fusionfix.io or in FusionFixHub, in English, French, German, Italian and Spanish; there is no in-game options menu. Camera tuning, Field of View, FPS Limit, Minimap Fix, Console Gamma and the right-stick deadzone apply at once, Anisotropic Filtering from the next level, the rest after a restart. Presets don't change window mode, sensitivities, invert look, horizontal-only stick look or the deadzone.

## Known limits


- HUD width constraints and separate movie display modes are not supported.
- The frame-rate setting changes the game’s race limit. Zero restores the game’s own value; there is no monitor-refresh option.
- Cutscenes at 21:9 and wider may reveal the edges of the scene; there is no letterbox option.
- The garage monitor "shake" in one cutscene also happens in the original at 30 FPS.
- No in-game settings menu; no controller icon or raw mouse changes (the game's own input is used).

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
