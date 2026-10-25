<div align="center">

<img src="https://thirteenag.github.io/screens/punisher/main2.jpg" width="760" alt="The Punisher Widescreen Fix">

**The Punisher Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling, field of view and video scaling for widescreen displays.

[Website](https://thirteenag.github.io/wfp#punisher) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ThePunisher.WidescreenFix/dllmain.cpp)

</div>

---

The included ASI loader provides Direct3D 8 to 9 conversion. Keep `d3d8.dll` and `d3d8.ini` together beside the game executable.

Adds custom resolution, widescreen camera correction, HUD and weapon-overlay alignment, and menu/video framing fixes. The menus retain their original proportions in a centered 4:3 area, with black side borders on wider screens.

Install the release in the game directory. Configure it in the game's menus, on fusionfix.io or in FusionFixHub.

## Settings

Open **Options → Plugins → The Punisher Widescreen Fix** for presets, video framing and intro skipping. **Game Settings** also contains **HUD Aspect Ratio** and **FOV Multiplier** beside the game's options.

Use **More Pages** and **Previous Pages** to browse a plugin's settings. **Details** describes the last selected option. **Back** returns to the previous page. Each plugin has its own preset and Restore Defaults action; those actions leave the joined Game Settings values unchanged. The game's confirmed **Restore Default Settings** action also resets the added Game Settings values. A restart notice appears when an option needs the game to restart.

| Setting | Values | Default |
|---|---|---|
| Resolution width and height | 0 = desktop resolution, or a size; Hub/settings file; restart | 0 |
| HUD aspect ratio | Auto for full HUD width, or 4:3, 16:9, 21:9/custom up to 32:9; live | Auto |
| FOV multiplier | 0.5–2.0; larger values widen the view; live | 1.0 |
| Video mode | Original aspect, Fill or Stretch; live | Fill |
| FPS limit | 0 = unlimited, -1 = desktop refresh, or 30–1000; live | 60 |
| Skip intro | On or Off; restart | On |

Vanilla disables the FPS limit, shows the intro and fits videos without cropping. Recommended and Enhanced skip the intro and fill the screen with video. Hub texts are available in English, French, German, Italian and Spanish.

Framed game windows open centered in the monitor work area and recenter when resized. HUD sprites retain their native proportions and pixel sizes, with panels anchored to the screen or selected HUD aspect.

## Known limits

- The field-of-view multiplier also affects scripted world cameras. Portraits and other menu views keep their original framing.
- Fill crops video edges and may crop text embedded in the video. Stretch changes video proportions.
- Window-mode, cutscene-border and anti-aliasing options are unavailable.
- 60 FPS is recommended. Higher frame rates may affect game behavior.
- The added in-game text is in English. Use fusionfix.io or FusionFixHub for the other languages.
- Other executables can use fusionfix.io, FusionFixHub or the settings file when the in-game menu is unavailable.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
