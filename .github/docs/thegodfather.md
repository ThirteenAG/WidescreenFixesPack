<div align="center">

<img src="https://thirteenag.github.io/screens/thegodfather/main2.jpg" width="760" alt="The Godfather Widescreen Fix">

**The Godfather Widescreen Fix** corrects resolution handling, HUD scaling and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thegodfather) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheGodfather.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Replaces the game's 18 resolution entries with modes reported by the monitor. When the list is full, lower modes are omitted; the widescreen preference can also omit 4:3 modes.
- **Hor+ field of view** — The 3D view keeps the original vertical field of view and widens horizontally; pedestrian culling is widened with it.
- **HUD and menus** — The HUD, text wrapping, mouse cursor and front-end player view keep a centered 4:3 layout; the radar is corrected; full-screen flashes still cover the screen.
- **Display scaling** — The game gets the full resolution on scaled desktops (125–300 %).

## Options

| Setting | Values | Default | Applies |
|---|---|---|---|
| Prefer widescreen resolutions | on/off | on | restart |
| Field of view factor | 0.5–2.0 | 1.0 | while playing |

Open **Options → Video** for **Prefer widescreen resolutions**. Choose **Plugins → The Godfather Widescreen Fix** for the field of view, presets and Restore defaults. Use the mouse or arrow keys and Enter; Escape goes back. The game’s Default button also resets the added Video option.

Presets: Vanilla turns *Prefer widescreen resolutions* off; Recommended and Enhanced turn it on. The field of view factor is 1.0 in every preset.

## Known limits

- A small part of the stock tab border remains visible on Plugins pages.
- After changing a restart-only setting, its restart notice can take the place of contextual help.
- The field of view factor also changes cutscene cameras: the game uses the same camera setup for both.
- The HUD stays in a centered 4:3 box; its width cannot be adjusted.
- Movies keep their original scaling.
- The Paramount startup logo cannot be skipped with this fix.
- No window mode setting.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/TheGodfather.WidescreenFix.cfg`.
4. Configure the fix in the game, on fusionfix.io or in FusionFixHub. Restart the game when an option requires it. An old `scripts/TheGodfather.WidescreenFix.ini` isn't used anymore and can be deleted.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>

