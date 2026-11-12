<div align="center">

<img src="https://thirteenag.github.io/screens/sniperelite/main2.jpg" width="760" alt="Sniper Elite Fusion Fix">

**Sniper Elite Fusion Fix** places the HUD at the screen edges, keeps menus, cinematics and videos in proportion and adds window modes, a frame rate limit and an intro skip.

[Website](https://thirteenag.github.io/wfp#sniperelite) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SniperElite.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Any resolution; 0 uses the desktop's.
- **Field of view** — The view widens with the screen (Hor+); the field of view factor widens it further, up to 150 degrees across.
- **HUD** — The compass, health and weapon panels sit at the edges of the screen, or in a centred box of the ratio you choose; their numbers move with them. Messages follow the box; menus, briefings, loading screens and the scope keep their proportions.
- **Cinematics** — The opening and story cinematics keep their original wide 2.35:1 frame at any screen shape: bars above and below on narrower screens, at the sides on wider ones, or none (the cinematic fills the screen).
- **Videos** — The intro and story movies keep their aspect ratio. Original shows them as the game does, whole inside its centred 4:3 frame with black bars around them; Fill fits the whole movie to the screen. Nothing is cut off or stretched.
- **Window modes** — Fullscreen, a window with exactly the chosen resolution, or borderless covering the monitor at its own resolution.
- **Cursor** — The cursor stays inside the game window while the game is active.
- **Skip intro** — The legal screen and the Rebellion logo movie are skipped at startup; the intro movie still plays and can be skipped with the game's own button.
- **Frame rate limit** — An even frame rate limit, 60 FPS by default.
- **Update notice** — With update checks on, a newer version is announced in one line on the game's menu screens.
- **Display scaling** — The game gets the full resolution on scaled desktops.

## Settings

Change the settings on fusionfix.io, in FusionFixHub or in the developer overlay (Ctrl+F10, for supporters). The first launch creates `scripts/SniperElite.FusionFix.cfg`.

| Setting | Values | Default | Applies |
|---|---|---|---|
| Window mode | Fullscreen, Windowed, Borderless | Fullscreen | after a restart |
| Pause in background | On, Off | On | while playing |
| Confine cursor | On, Off | On | while playing |
| Skip intro logos | On, Off | On | after a restart |
| Check for updates | On, Off | Off | while playing |
| HUD aspect ratio | Auto, 4:3, 16:9, 21:9, 32:9 | Auto | while playing |
| Cutscene borders | Off, Letterbox, Pillarbox, Both | Both | while playing |
| Video mode | Original aspect, Fill | Fill | while playing |
| Field of view factor | 0.5–2.0 | 1.0 | while playing |
| Frame rate limit | Unlimited, monitor refresh rate, 30–1000 FPS | 60 | while playing |
| Resolution (width, height) | pixels; 0 = desktop | 0 | after a restart |

Presets: Vanilla keeps the original letterbox bars, shows the intro screens, plays videos at their original aspect and leaves the frame rate unlimited; Recommended and Enhanced use both border types, skip the intro screens, fit videos to the screen and limit the game to 60 FPS. Field of view and HUD aspect ratio keep their current values when switching presets; their defaults are 1.0 and Auto.

## Known limits

- No page in the game's own settings menu: its menu pages come from the game's packed menu data, so the fix's settings are on fusionfix.io, in FusionFixHub and in the overlay.
- The title screen still waits for "Start Game".
- No controller support: the game polls a DirectInput pad but never maps it to anything, so play with keyboard and mouse.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/SniperElite.FusionFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
