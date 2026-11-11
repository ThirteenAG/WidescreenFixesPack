<div align="center">

<img src="https://thirteenag.github.io/screens/justcause/main2.jpg" width="760" alt="Just Cause Fusion Fix">

**Just Cause Fusion Fix** places the HUD at the screen edges, keeps menus, cinematics and videos in proportion and adds a field of view factor, window modes, a frame rate limit and an intro skip.

[Website](https://thirteenag.github.io/wfp#justcause) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/JustCause.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Any resolution; 0 uses the desktop's.
- **Field of view** — The view widens with the screen; the field of view factor widens it further, up to 150 degrees across. Cinematics keep their framing.
- **HUD** — The health bar and weapon selector sit at the left edge, the minimap with its markers, the meter beside it and the distance display at the right edge, or in a centred box of the ratio you choose. Objectives, messages and subtitles stay in the central area.
- **Menus and loading screens** — Kept at their original shape in the middle of the screen.
- **Cinematics** — The in-game cinematic bars are kept (Letterbox), the picture is limited to its original 4:3 shape between side bars (Pillarbox), both, or none so the cinematic fills the screen.
- **Videos** — The story videos keep their aspect ratio and are always shown whole, with black bars where needed.
- **Window modes** — Fullscreen, a window with exactly the chosen resolution, or borderless covering the monitor.
- **Background** — The game can keep running while another window is active; the cursor stays inside the game window while the game is active.
- **Skip intro** — The Avalanche and Eidos logo videos are skipped at startup.
- **Frame rate limit** — An even frame rate limit, 60 FPS by default.
- **Controller** — The bundled Xidi presents an Xbox controller to the game's own controller support.
- **Update notice** — With update checks on, a newer version is announced in one line on the game's menu screens.
- **Display scaling** — The game gets the full resolution on scaled desktops.

## Settings

Change the settings on fusionfix.io, in FusionFixHub or in the developer overlay (Ctrl+F10, for supporters). The first launch creates `scripts/JustCause.FusionFix.cfg`.

| Setting | Values | Default | Applies |
|---|---|---|---|
| Window mode | Fullscreen, Windowed, Borderless | Fullscreen | after a restart |
| Pause on focus loss | On, Off | On | while playing |
| Confine cursor | On, Off | On | while playing |
| Skip intro | On, Off | On | after a restart |
| Movie display | Original, Fill | Fill | while playing |
| HUD aspect ratio | Auto, 4:3, 16:9, 21:9, 32:9 | Auto | while playing |
| Frame rate limit | Unlimited, monitor refresh rate, 30–1000 FPS | 60 | while playing |
| Field of view factor | 0.5–2.0 | 1.0 | while playing |
| Cutscene borders | Off, Letterbox, Pillarbox, Both | Both | while playing |
| Check for updates | On, Off | Off | while playing |
| Resolution (width, height) | pixels; 0 = desktop | 0 | after a restart |

Presets: Vanilla keeps the original cinematic bars, shows the logo videos and leaves the frame rate unlimited; Recommended and Enhanced use both border types, skip the logos and limit the game to 60 FPS. Videos always retain their aspect ratio. Field of view and HUD aspect ratio keep their current values when switching presets; their defaults are 1.0 and Auto.

## Known limits

- No page in the game's own settings menu: its menu pages come from the game's packed menu data, so the fix's settings are on fusionfix.io, in FusionFixHub and in the overlay.
- The controller uses the game's own controller bindings and button names.
- The resolution is set by the fix; the game's own resolution list still shows its original entries.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/JustCause.FusionFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
