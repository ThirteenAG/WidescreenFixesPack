<div align="center">

<img src="https://thirteenag.github.io/screens/thematrixpathofneo/main2.jpg" width="760" alt="The Matrix: Path of Neo Fusion Fix">

**The Matrix: Path of Neo Fusion Fix** corrects resolution handling, aspect ratio and field of view for widescreen displays, keeps the HUD, menus, cutscenes and movies in proportion, adds window modes, a controller through Xidi, and skips the copyright screen.

[Website](https://thirteenag.github.io/wfp#thematrixpathofneo) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheMatrixPathOfNeo.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Renders at the desktop resolution, or at the resolution set in the fix's options, instead of the game's short list of 4:3 modes.
- **Aspect ratio and field of view** — Gameplay keeps the original vertical view and shows more at the sides (Hor+); at 4:3 it is unchanged.
- **HUD** — Health, Focus, the weapon and ammunition display and the allies' portraits sit in the top left corner of the screen, the operator's radio and the counters in the top right corner, without being stretched; the aiming reticle, prompts and subtitles stay in the middle.
- **Menus and 2D screens** — Menus, the copyright screen and the pause screen keep their 4:3 layout in the middle of the screen.
- **Cutscenes** — The game's realtime cutscenes show their original 16:9 framing at any aspect ratio.
- **Movies** — The full-motion videos keep their proportions.
- **Stability** — A crash that could happen right at start-up (the loading screen thread starting too early) is avoided.
- **Display scaling** — The game stays sharp on scaled (high-DPI) desktops.
- **Controller** — The bundled Xidi presents an Xbox-compatible controller to the game's own DirectInput controller support.

## Options

- **Window mode** — Fullscreen, Windowed or Borderless, then restart. Windowed gives a centred window whose picture area is exactly the chosen resolution; Borderless covers the monitor. Alt+Enter switches between fullscreen and the window.
- **HUD aspect ratio** — Auto (default) puts the HUD in the screen corners; a ratio such as 4:3 or 16:9 keeps it in a centred box of that shape. Applies at once.
- **Cutscene borders** — Letterbox keeps the bars above and below cutscenes on screens narrower than 16:9; Pillarbox keeps the 16:9 picture between side bars on wider screens; Both (default) does both and always shows the original framing; Off fills the screen. Applies at once.
- **Movie framing** — Original shows them as the game does, whole inside its centred 4:3 frame with black bars around them; Fill fits the whole movie to the screen. Nothing is cut off or stretched (Fill is the default). Applies at once.
- **Field of view** — A multiplier (0.5–2.0, default 1.0) on the gameplay camera, limited to 150 degrees horizontally; cutscenes and scripted cameras keep their own. Applies at once.
- **Skip intro** — On by default: the copyright screen at start-up is skipped. Needs a restart.
- **Pause in background** — On by default: in a window mode the game pauses while another window is active. Off keeps it running.
- **Confine cursor** — Keeps the mouse cursor inside the game window while the game is active.
- **Check for updates** — Off by default. When on, the fix asks fusionfix.io once per start whether a newer version is out and shows a notice in the game's menus; FusionFixHub opens when you close the game.
- **Resolution width / height** — Only in the cfg and FusionFixHub's text editor. 0 uses the desktop resolution.

---

## Settings and hub

The fix creates `scripts/TheMatrixPathOfNeo.FusionFix.cfg` on first start; FusionFixHub and fusionfix.io can change it, also while the game runs. When a changed setting needs a restart, the game's menus say so at the top of the screen. Vanilla keeps the copyright screen, the game's own cutscene bars and the movies with black bars; Recommended and Enhanced skip the copyright screen, keep the original cutscene framing with side bars on wider screens and fill the screen with the movies. Window, resolution and input settings aren't changed by presets. Texts are available in English, French, German, Italian and Spanish.

## Known limits

- The game's menus are built by its own scripts, so the fix has no page in them: change its settings in FusionFixHub or on fusionfix.io.
- The game runs at 30 frames per second, as it always has; the fix doesn't change its frame rate.
- The button prompts show keyboard and mouse keys also when a controller is used.
- The window can't be resized by dragging; pick the size in the fix's options.
- The startup logo screens are already skipped by the game itself; Skip intro removes the copyright screen.
- The mouse is read through DirectInput without Windows pointer acceleration, so no raw input option is needed.

## Installation

1. Download the fix archive from this release.
2. Extract it into the game folder, beside the game executable, keeping the folders in the archive.
3. Start the game once to create `scripts/TheMatrixPathOfNeo.FusionFix.cfg`.
4. Change settings in FusionFixHub or on fusionfix.io. Restart when an option requires it.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
