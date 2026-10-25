<div align="center">

<img src="https://thirteenag.github.io/screens/thaw/main2.jpg" width="760" alt="Tony Hawk&#x27;s American Wasteland Widescreen Fix">

**Tony Hawk's American Wasteland Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling, and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thaw) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksAmericanWasteland.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution and aspect ratio** — the desktop resolution by default (a resolution set in the fix's options overrides it), correct aspect at any width; the resolution-dependent color table is sized correctly.
- **Field of view** — Hor+ for the gameplay camera; `Field of view` multiplies it (0.5–2.0, live) on normal follow cameras only.
- **HUD** — the HUD and menus keep their proportions; `HUD aspect ratio` keeps the side elements in a centered box (live).
- **Window mode** — fullscreen, windowed or borderless; sharp on scaled desktops, square corners.
- **Skip intro logos** — publisher, studio, Aspyr and technology logos only; story videos stay.
- **Soundtrack** — random song order.

## Options

Open **Plugins** in the game's options menu, then select **Tony Hawk's American Wasteland Widescreen Fix**. HUD correction and HUD aspect ratio are beside the game's Display options. Left and right change a value; Back returns to the previous page.

Settings can also be changed in FusionFixHub (changes apply while the game runs) or on fusionfix.io. Texts are in English, French, German, Italian and Spanish.

Presets: Vanilla keeps the corrections and turns off skip intro and random song order; Recommended (default) and Enhanced turn them on.

## Known limits

- Videos keep the game's original scaling.
- Alt+Enter and resizing the window while running are not handled; window mode applies at start.
- Split-screen HUD placement and fullscreen HUD layers are unchanged.
- Controller support and button prompts follow the game's original behavior.

## Installation

1. Extract the archive into the game's installation folder, keeping its folders. The included files belong under `Game`.
2. Start the game once: `Game/scripts/TonyHawksAmericanWasteland.WidescreenFix.cfg` is created.
3. Change settings in FusionFixHub (also while the game runs). Settings marked as such apply at the next start. An old `Game/scripts/TonyHawksAmericanWasteland.WidescreenFix.ini` isn't used anymore and can be deleted.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
