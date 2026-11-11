<div align="center">

<img src="https://thirteenag.github.io/screens/callofcthulhu/main2.jpg" width="760" alt="Call of Cthulhu: Dark Corners of the Earth Fusion Fix">

**Call of Cthulhu: Dark Corners of the Earth Fusion Fix** gives the game a proper widescreen view, keeps menus, cutscenes and videos in proportion and adds window modes, a frame rate limit and an in-game settings page.

[Website](https://thirteenag.github.io/wfp#callofcthulhu) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/CallOfCthulhu.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **View** — The game keeps its vertical view at every aspect ratio, so wider screens see more at the sides. The field of view factor widens the normal gameplay view, up to 150 degrees across.
- **Menus** — Menus, the inventory, the journal and loading screens keep their 4:3 layout in the middle of the screen, with black beside full screen pictures. Dimming behind menus, fades, subtitle bands and tip panels cover the whole screen.
- **Cutscenes** — The in-engine cutscenes are framed as a 16:9 picture (the original bars on a 4:3 screen). The fix keeps that picture at any screen shape: on 16:9 it fills the screen, on wider screens it stays 16:9 between side bars, on narrower ones it gets bars above and below.
- **Videos** — Videos keep their aspect ratio and are always shown complete, with black bars where needed. Original shows them inside the game's centred 4:3 frame, Fill fits them to the screen; a 4:3 video looks the same in both.
- **Window modes** — Fullscreen, a window with exactly the chosen resolution, or borderless at the resolution of the monitor the window is on.
- **Frame rate** — The game runs uncapped at hundreds of frames per second; the frame rate limit holds it at 60 or the rate you choose.
- **Background** — The game can keep running while another window is active, and the cursor stays inside the window while the game is active.
- **Skip intro** — The startup logo and legal videos are skipped.
- **High resolutions** — Menus and pictures draw correctly on screens wider than 3276 pixels, where the game's own 2D drawing ran out of range.
- **Crash fix** — The heal menu's cursor fix no longer writes into the game's read-only data (a crash at startup).
- **Display scaling** — The game gets the full resolution on scaled desktops.

## Options

Open **Options → Plugins → Call of Cthulhu Fusion Fix** from the title screen or the pause menu. Choose a row and press Enter for its next value. The line at the bottom explains the selected row and says when a change needs a restart.

| Setting | Values | Default | Applies |
|---|---|---|---|
| Preset | Vanilla, Recommended, Enhanced | Recommended | with Apply preset |
| Window mode | Fullscreen, Windowed, Borderless | Fullscreen | after a restart |
| Field of view | 0.5–2.0 | 1.0 | while playing |
| Video framing | Original aspect, Fill | Fill | while playing |
| Cutscene borders | Off, Letterbox, Pillarbox, Both | Both | while playing |
| Frame rate limit | Unlimited, monitor refresh rate, 30–1000 FPS | 60 | while playing |
| Confine cursor | On, Off | On | while playing |
| Pause on focus loss | On, Off | On | while playing |
| Skip intro | On, Off | On | after a restart |
| Check for updates | On, Off | Off | while playing |

Cutscene borders: Both keeps the 16:9 cutscene picture at every screen shape; Letterbox only adds bars above and below (on wide screens the picture then fills the width), Pillarbox only at the sides; Off removes the bars and shows the wider gameplay view.

Presets: Vanilla shows the intro, uses Letterbox for cutscenes, plays videos at their original aspect and leaves the frame rate unlimited; Recommended and Enhanced skip the intro, keep the 16:9 cutscene picture, fit videos to the screen and limit the game to 60 FPS. The field of view factor is 1.0 in every preset.

With Check for updates on, a newer version of the fix is shown in the help line of the fix's page. Ctrl+F10 opens the developer overlay for supporters.

The settings text is available in English, French, German, Italian and Spanish; Automatic uses English, and Text Language in FusionFixHub picks another.

## Known limits

- The game has no on-screen HUD in play (no health, ammunition or map display); subtitles and messages stay centred, so there's no HUD setting.
- The PC version uses DirectInput for keyboard and mouse; controller input is not supported.
- Aiming, scripted views, camera effects and transitions keep their original framing; the factor changes the normal gameplay view.
- Long help lines on the in-game page are shortened; FusionFixHub and fusionfix.io show them in full.
- On screens narrower than 4:3 (5:4) the 4:3 menus are slightly wider than the screen at the sides.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game's installation folder, preserving its directory structure. The included files belong under `Engine`.
3. Launch the game once to create `Engine/scripts/CallOfCthulhu.FusionFix.cfg`.
4. Configure the fix in the game (Options → Plugins), on fusionfix.io or in FusionFixHub.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
