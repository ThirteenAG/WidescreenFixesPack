<div align="center">

<img src="https://thirteenag.github.io/screens/larush/main2.jpg" width="760" alt="L.A. Rush Fusion Fix">

**L.A. Rush Fusion Fix** corrects resolution handling, aspect ratio, HUD scaling, field of view and video scaling for widescreen displays.

[Website](https://thirteenag.github.io/wfp#larush) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/LARush.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Renders at the desktop resolution, or at the resolution set in the fix's options (out-of-range values fall back to the desktop).
- **Aspect ratio** — Correct widescreen projection.
- **HUD and menus** — The minimap, speedometer, top indicators and race timer, lap and position can be positioned at the screen edges or inside a centered HUD box. Menus, loading screens, the pause pages and the big map keep their original 4:3 proportions with black sides.
- **Controller** — An Xbox-style controller works out of the box through the bundled Xidi: A confirms and B goes back in menus, and the joystick bindings screen shows controller button names.
- **Videos** — Original fits the whole video in the original 4:3 presentation. Fill shows each whole video as large as the screen allows and zooms past the black bars encoded in the intro movie.
- **Startup** — The game's CD checks for its movies are handled, so the game runs from the hard disk.
- **Display scaling** — The game stays sharp on scaled (high-DPI) desktops.

## Options

- **Skip intro logos** (restart) — Vanilla off, Recommended and Enhanced on. Skips publisher, studio and technology logos and the legal notice. The story intro remains available and can be skipped normally.
- **Skip title prompt** (restart) — Plays the intro movie first, then advances past the initial title prompt. Vanilla off, Recommended and Enhanced on.
- **Resolution width/height** (restart).
- **Window mode** — Fullscreen, Windowed or Borderless (restart). Borderless uses the desktop resolution.
- **Field of view** — 0.5–2.0; 1.0 preserves the intended driving view. The opening special camera retains its framing.
- **HUD aspect ratio** — Auto or a centered aspect-ratio box; applies live.
- **Cutscene borders** — Pillarbox keeps story scenes and race intros in their original 4:3 framing; Off shows them wider. Applies live.
- **Menu frame limit** — Applies in menus, including pause. 0 disables the additional limit without changing the game's native timing (Vanilla); Recommended and Enhanced use 60. Values from 1 to 29 use 30; driving and loading retain the game's native timing.
- **Controller labels** — Selects Xbox, PlayStation or Nintendo button names on the joystick bindings screen. Auto uses Xbox names; applies live.
- **Pause when unfocused** — Pauses driving while another window is active; applies live.
- **Confine cursor** — Keeps the cursor inside the focused window during gameplay and releases it when the game shows a menu cursor; applies live.
- **Check for updates** — Optional update notice at the main menu; disabled by default.

Settings are changed on fusionfix.io or in FusionFixHub. Defaults are the Recommended preset; Vanilla and Enhanced are the other presets. No preset changes the resolution. The fix has no in-game menu. Supporters can open the developer overlay with Ctrl+F10 or Start and View together on a controller.

## Known limits

- The fix has no page in the game's own options menu; its settings are changed in FusionFixHub or on fusionfix.io.
- The title prompt ("Press SPACE to Start") always names the keyboard key; Skip title prompt passes it by default.
- The phone shown during story calls is part of the 3D view and keeps its original place left of the center.
- The window size follows the resolution chosen at startup; resizing the window needs a restart.

## Installation

1. Download the fix archive from this release.
2. Extract it into the game folder, beside the game executable, keeping the folders in the archive.
3. Start the game once to create `plugins/LARush.FusionFix.cfg`.
4. Change settings on fusionfix.io or in FusionFixHub, then restart the game.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
