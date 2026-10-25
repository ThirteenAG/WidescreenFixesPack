<div align="center">

<img src="https://thirteenag.github.io/screens/psychonauts/main2.jpg" width="760" alt="Psychonauts Widescreen Fix">

**Psychonauts Widescreen Fix** corrects widescreen HUD and video scaling.

[Website](https://thirteenag.github.io/wfp#psychonauts) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Psychonauts.WidescreenFix/dllmain.cpp)

</div>

---

Corrects widescreen HUD, full-screen effects and movie placement, and selects desktop resolution as the native default.

## Settings

Configure on fusionfix.io or in FusionFixHub; Window mode, field of view, HUD width, video mode and pausing when unfocused change while the game runs. Intro skipping needs a restart.

| Setting | Values | Default |
|---|---|---|
| Window mode | Fullscreen, Windowed, Borderless (while playing); Alt+Enter switches modes | Fullscreen |
| Field of view | 0.5-2.0, normal gameplay camera (while playing) | 1.0 |
| HUD aspect ratio | Auto (whole screen width), 4:3, 16:9, 21:9 or any width:height | Auto |
| Video mode | Original aspect, Fill (cropped), Stretch | Fill |
| Skip intro | Skips the startup logo | On |
| Pause on focus loss | Pauses when another window is active; off keeps the game running | On |

Vanilla shows the startup logo; Recommended and Enhanced skip it. Vanilla keeps videos at their original aspect; Recommended and Enhanced crop them to fill the screen. The pause setting is a player choice and isn't changed by presets. Texts are available in English, French, German, Italian and Spanish.

## Known limits

The FOV multiplier changes the normal gameplay and ball cameras at their standard angle. Scripted cameras, zoom, map views and FOV transitions keep their original framing.


- No cutscene-border setting.
- No anti-aliasing option and no in-game options menu; use fusionfix.io or FusionFixHub.

## Installation

Extract the release archive into the game folder, preserving its folders. Launch once to create `scripts/Psychonauts.WidescreenFix.cfg`, then configure the fix in FusionFixHub or on fusionfix.io.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
