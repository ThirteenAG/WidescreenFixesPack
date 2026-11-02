<div align="center">

<img src="https://thirteenag.github.io/screens/psychonauts/main2.jpg" width="760" alt="Psychonauts Fusion Fix">

**Psychonauts Fusion Fix** places the HUD at the screen edges, keeps menus and videos in proportion and adds window modes and an in-game settings page.

[Website](https://thirteenag.github.io/wfp#psychonauts) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Psychonauts.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **HUD** — In play every HUD element sits at its edge of the screen (health and lives on the left; rank, figments, cards and powers on the right), or in a centred box of the ratio you choose. Goal messages stay centred; the journal menus and loading screens keep their 4:3 layout.
- **Field of view** — The game keeps its vertical view at every aspect ratio; the field of view factor widens the normal gameplay camera, up to 150 degrees across.
- **Videos** — The intro and story movies keep their aspect ratio and are always shown whole at 4:3 between black bars.
- **Window modes** — Fullscreen, a window with exactly the chosen resolution, or borderless covering the monitor at its own resolution; switched while playing.
- **Background** — The game can keep running while another window is active, and the cursor stays inside the window while the game is active.
- **Skip intro** — The Double Fine logo movie is skipped, and the title screen goes to the menu without "Press ... to begin".
- **Display scaling** — The game gets the full resolution on scaled desktops.

Applies to the retail and Steam versions of Psychonauts.

## Options

In Psychonauts, open the journal (Esc) and choose **Options → Plugins → Psychonauts Fusion Fix**, from the title screen or while playing. Point at a row and press Enter (or the pad's accept button) for its next value; Left and Right step either way. The line at the bottom explains the row under the cursor and says when a change needs a restart. Back returns to the game's options.

| Setting | Values | Default | Applies |
|---|---|---|---|
| Preset | Vanilla, Recommended, Enhanced | Recommended | with Apply preset |
| Window mode | Fullscreen, Windowed, Borderless | Fullscreen | while playing |
| Pause when unfocused | On, Off | On | while playing |
| Confine cursor | On, Off | On | while playing |
| Skip intro | On, Off | On | after a restart |
| Skip press start | On, Off | On | while playing |
| Check for updates | On, Off | Off | while playing |
| Settings language | Language of the Plugins page; FusionFixHub/settings file | Automatic | when the menu opens |
| HUD aspect ratio | Auto, 4:3, 16:9, 21:9, 32:9 | Auto | while playing |
| Field of view | 0.5–2.0 | 1.0 | while playing |
| Frame rate limit | Unlimited, monitor refresh rate, 30–1000 FPS | 60 | while playing |

Presets: Vanilla shows the logo and the title prompt and leaves the frame rate unlimited; Recommended and Enhanced skip both and limit the game to 60 FPS. The field of view factor is 1.0 and the HUD Auto in every preset.

Ctrl+F10 opens the developer overlay for supporters.

## Known limits

- The story cutscenes are 4:3 movies without letterbox bars, so there's no cutscene-border setting; they're shown whole at 4:3.
- Scripted cameras, zoom, map views and field of view transitions keep their original framing; the factor changes the normal gameplay and ball cameras.
- The game reads the controller itself; the fix adds no controller handling.
- A custom window size appears alongside the monitor's display modes in Options → Graphics.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/Psychonauts.FusionFix.cfg`.
4. Configure the fix in the game's journal (Options → Plugins), on fusionfix.io or in FusionFixHub.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
