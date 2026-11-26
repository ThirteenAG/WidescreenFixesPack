<div align="center">

<img src="https://thirteenag.github.io/screens/enterthematrix/main2.jpg" width="760" alt="Enter the Matrix Fusion Fix">

**Enter the Matrix Fusion Fix** brings correct widescreen views, an edge-anchored HUD, whole movies, cutscene borders and proper window modes to Enter the Matrix.

[Website](https://thirteenag.github.io/wfp#enterthematrix) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/EnterTheMatrix.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — MatrixOptions lists every display mode; the game renders at the resolution saved in its own `MatrixConfig.ini` (the desktop resolution when none is saved or an old numeric resolution index is present).
- **Aspect ratio** — A wider view on widescreen screens (Hor+), with nothing disappearing at the screen sides.
- **HUD** — Health, Focus and the weapon display sit at the screen edges; their size and shape stay as in the original.
- **Menus and 2D screens** — Menus, pause screens, loading screens, subtitles and messages keep their original 4:3 layout in the middle of the screen; on screens narrower than 4:3 they get black bars above and below instead of being cut off.
- **Movies** — Every movie is shown whole and undistorted.
- **Cutscenes** — In-game cutscenes keep their original 16:9 framing with black borders around it, and can be switched to fill the screen.
- **Window** — Windowed mode centers the game at the chosen client resolution with square corners on Windows 11. At desktop resolution the client covers the monitor and the frame stays outside the screen after minimize and restore.
- **Display scaling** — The game is sharp on scaled (high-DPI) desktops; MatrixOptions keeps Windows' own dialog scaling.
- **Controller** — The package includes Xidi, so an Xbox-style controller shows up as the game's own DirectInput controller.
- **Mouse input** — Raw mouse reports are collected in batches on a separate thread. The game reads the accumulated movement, buttons and wheel through its usual controls.

## Options

- **Window mode** (restart) — Fullscreen, Windowed or Borderless. Windowed uses the resolution chosen in MatrixOptions; Borderless covers the monitor at its own resolution.
- **HUD aspect ratio** — Auto puts the HUD at the screen edges. Choosing a ratio keeps it in a centered box of that shape. Auto is the default in all three Hub preset exports. Applies immediately.
- **Cutscene borders** — Both (default) always shows the original 16:9 cutscene framing with bars where the screen is wider or narrower; Letterbox keeps only the bars above and below (as the original on 4:3 screens); Pillarbox keeps only the side bars; Off fills the screen. Vanilla uses Letterbox. Applies immediately, also during a cutscene.
- **Frame rate limit** — Choose 30–1000 FPS, the monitor refresh rate, or Off to leave the game's own pacing in effect. Recommended and Enhanced add a 60 FPS cap. The game or VSync may impose a lower limit. Applies immediately.
- **Movie framing** — Original aspect shows the movies as the game does, whole inside the centered 4:3 frame (Vanilla). Fill shows each whole movie as large as the screen allows without cutting anything off (Recommended, Enhanced). Applies immediately.
- **Field of view factor** — 0.5–2.0. At 1.0 the ordinary gameplay camera keeps its intended view. Cutscenes, sniper scopes and binoculars keep their original framing. Applies immediately.
- **Skip intro** (restart) — Skips the logo movies and the copyright notice; the game opens at the title menu. Vanilla off, Recommended and Enhanced on.
- **Pause in background** — In Windowed and Borderless the game pauses while another window is active, as the original does. Off keeps it running. Applies immediately.
- **Confine cursor** — Keeps the cursor inside the active game window. Applies immediately.
- **Check for updates** — Optional update notice on the title screen; off by default.

Settings are changed on fusionfix.io or in FusionFixHub. Defaults are set by the Recommended preset, alongside Vanilla and Enhanced. The fix supports `Matrix.exe` (1.0), `Matrix12.exe` and the DVD release's `Matrix.exe` (1.2), `MatrixOptions.exe` and `MatrixOptions12.exe`.

## Known limits

- The fix has no page in the game's own options menu (its option pages are fixed screens with no list to add to); its settings are changed in FusionFixHub or on fusionfix.io.
- The window size follows the resolution chosen at startup; resizing the window and Alt+Enter aren't supported by the game.
- Button prompts show keyboard keys while a controller is used.
- In Fill, the subtitles of the story movies are shown over the bottom of the picture instead of below it.

## Installation

1. Download the fix archive from this release.
2. Extract it into the game folder, beside the game executable, keeping the folders in the archive.
3. Start the game once to create `scripts/EnterTheMatrix.FusionFix.cfg`.
4. Change settings on fusionfix.io or in FusionFixHub, then restart the game.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark&layout=mobile"><source media="(max-width: 768px)" srcset="https://fusionfix.io/credits/supporters.svg?layout=mobile"><source media="(prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark"><img width="100%" src="https://fusionfix.io/credits/supporters.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
