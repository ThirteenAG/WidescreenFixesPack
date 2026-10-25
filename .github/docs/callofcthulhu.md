<div align="center">

<img src="https://thirteenag.github.io/screens/callofcthulhu/main2.jpg" width="760" alt="Call of Cthulhu: Dark Corners of the Earth Widescreen Fix">

**Call of Cthulhu: Dark Corners of the Earth Widescreen Fix** corrects widescreen aspect ratio, HUD, and video scaling. The known issues linked below still apply.

[Website](https://thirteenag.github.io/wfp#callofcthulhu) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/CallOfCthulhu.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Videos** — Correct FMV scaling for widescreen.

## Options

- **Gameplay FOV** - Adjust the normal gameplay view from 0.5 to 2.0, with 1.0 as the default. Aiming, scripted views and camera transitions keep their own framing.
- **Video framing** - Choose Original aspect with black bars, cropped Fill (the default), or Stretch.
- **Window Mode** - Choose Fullscreen, Windowed or Borderless, then restart. Windowed mode uses a fixed window size; change the resolution to change its size.
- **Skip Intro** - Skip startup logos while retaining story and bonus movies.
- **Pause on Focus Loss** - Choose whether the game pauses automatically when you switch to another application.
- **Fullscreen images** - Choose whether fullscreen images stretch to fill the screen.

FOV, video framing and Pause on Focus Loss can be changed while playing through FusionFixHub. Open Options → Plugins from the title screen or pause menu to change the added settings.

## Known issues

Long menu help is shortened; FusionFixHub shows the full descriptions. The added pages use plain text without the usual menu panel background. With Fill Fullscreen Images off, unused areas beside menu artwork can appear gray.

Some original game issues remain; see [the issue list](https://github.com/ThirteenAG/WidescreenFixesPack/issues/420).

## Configuration

The added settings text is available in English, French, German, Italian and Spanish. Automatic uses English; select Text Language in FusionFixHub to use another language.

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. If Plugins is unavailable in your game version, use FusionFixHub or the generated `.cfg` file.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game’s installation folder, preserving its directory structure. The included files belong under `Engine`.
3. Launch once to create `Engine/scripts/CallOfCthulhu.WidescreenFix.cfg`, then change settings in FusionFixHub or on fusionfix.io.
4. Launch the game.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
