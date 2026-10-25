<div align="center">

<img src="https://thirteenag.github.io/screens/thps4/main2.jpg" width="760" alt="Tony Hawk&#x27;s Pro Skater 4 Widescreen Fix">

**Tony Hawk's Pro Skater 4 Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling, and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thps4) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksProSkater4.WidescreenFix/dllmain.cpp)

</div>

---

The included ASI loader provides Direct3D 8 to 9 conversion. Keep `d3d8.dll` and `d3d8.ini` together beside the game executable.

## Fixes

- **Resolution** — Set the output resolution on fusionfix.io or in FusionFixHub.
- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.

## Options

- **Resolution** - Choose a resolution, or leave width and height at 0 to use the desktop resolution.
- **Window Mode** - Choose Fullscreen, Windowed or Borderless, then restart. Borderless uses the desktop resolution.
- **Gameplay FOV** - Adjust the normal following camera from 0.5 to 2.0; 1.0 is neutral. Scripted and fixed views keep their own framing.
- **HUD Aspect Ratio Constraint** - Keep the single-player HUD at the screen edges, or within a centered area such as 16:9 or 21:9. Central indicators stay centered. Split-screen placement keeps its original layout.
- **Video Widescreen Mode** - Choose Original aspect with black bars, cropped Fill or Stretch.
- **Skip Intro** - Skip startup logo movies while retaining the game's main intro and other content movies.
- **Confine Cursor** - Keep the pointer inside the active game window; it is released when the game loses focus.
- **Frame Rate Limit** - The fix limits presentation to 60 FPS by default; 0 disables its cap and -1 uses the current desktop refresh rate. Other limits range from 30 to 1000. Restart after changing it. Higher frame rates can break gameplay. The cap is applied by the fix when the game presents a frame.
- **Soundtrack** - Randomize song order. Restart after changing this option.

Gameplay FOV, HUD width, video framing and cursor confinement can be changed while playing in FusionFixHub. Resolution and HUD correction require a restart.

Alt+Enter switches between fullscreen and the last windowed mode.

## Configuration

Open Options → Plugins to change the added settings in game. Long titles and help are shortened; FusionFixHub shows their full text. Plugins appears below Done, outside the original panel background. The added pages use plain text over the scene. If your Back shortcut does not leave a page, select its Back row.

The added settings text is available in English, French, German, Italian and Spanish. Choose Text Language in FusionFixHub to override the automatic choice. If the game language cannot be identified, English is used.

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. If Plugins is unavailable in your game version, use FusionFixHub or the generated `.cfg` file.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game’s installation folder, preserving its directory structure. The included files belong under `Game`.
3. Launch the game once to create the `Game/scripts/TonyHawksProSkater4.WidescreenFix.cfg`.
4. Configure the fix in FusionFixHub or on fusionfix.io. Restart the game when an option requires it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
