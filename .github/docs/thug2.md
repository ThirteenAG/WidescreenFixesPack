<div align="center">

<img src="https://thirteenag.github.io/screens/thug2/main2.jpg" width="760" alt="Tony Hawk&#x27;s Underground 2 Widescreen Fix">

**Tony Hawk's Underground 2 Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling, and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thug2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksUnderground2.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution and aspect ratio** — the desktop resolution by default (a resolution set in the fix's options overrides it), correct aspect at any width.
- **Field of view** — Hor+ for the gameplay camera; `Field of view` multiplies it (0.5–2.0, live) on normal follow cameras only.
- **HUD** — the HUD and menus keep their proportions; `HUD aspect ratio` keeps the side elements in a centered box (live).
- **Videos** — fullscreen Bink videos in their original aspect, filled (cropped) or stretched (live).
- **Window mode** — fullscreen, windowed or borderless; sharp on scaled desktops, square corners.
- **Skip intro logos** — publisher, studio and technology logos only; story videos stay.
- **Keyboard without the launcher** — when the launcher never saved controls, the launcher's default keys are used (the registry is not written).
- **Soundtrack** — random song order.

## Options

Open Game settings → Plugins from the title screen, or Options → Plugins while paused. Select Tony Hawk's Underground 2 Widescreen Fix to change its settings, choose a preset or restore its defaults. FusionFixHub also lets you change settings while the game runs. Texts are in English, French, German, Italian and Spanish.

Presets: Vanilla keeps the corrections and turns off skip intro, random song order and filled videos; Recommended (default) and Enhanced turn them on.

## Known limits

- Alt+Enter and resizing the window while running are not handled; window mode applies at start.
- On the title screen, Plugins uses a separate background strip beneath the original settings panel.
- Split-screen HUD placement and fullscreen HUD layers are unchanged.
- No controller icons; the game's own pad support is used.

## Installation

1. Extract the archive into the game's installation folder, keeping its folders. The included files belong under `Game`.
2. Start the game once: `Game/scripts/TonyHawksUnderground2.WidescreenFix.cfg` is created.
3. Change settings under Plugins or in FusionFixHub. Settings marked as such apply at the next start. An old `Game/scripts/TonyHawksUnderground2.WidescreenFix.ini` isn't used anymore and can be deleted.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
