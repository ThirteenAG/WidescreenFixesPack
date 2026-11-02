<div align="center">

<img src="https://thirteenag.github.io/screens/punisher/main2.jpg" width="760" alt="The Punisher Fusion Fix">

**The Punisher Fusion Fix** corrects resolution handling, aspect ratio, HUD scaling, field of view and video scaling for widescreen displays.

[Website](https://thirteenag.github.io/wfp#punisher) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ThePunisher.FusionFix/dllmain.cpp)

</div>

---

The included ASI loader provides Direct3D 8 to 9 conversion. Keep `d3d8.dll` and `d3d8.ini` together beside the game executable.

Adds custom resolution, widescreen camera correction, HUD and weapon-overlay alignment, and menu/video framing fixes. The menus retain their original proportions in a centered 4:3 area, with black side borders on wider screens.

Install the release in the game directory. Configure it in the game's menus, on fusionfix.io or in FusionFixHub.

## Settings

Open **Options → Plugins → The Punisher Fusion Fix** for presets, window mode, cutscene borders, intro skipping and the update check. **Game Settings** also contains **HUD Aspect Ratio** and **FOV Multiplier** beside the game's options.

Use **More Pages** and **Previous Pages** to browse a plugin's settings. **Details** describes the last selected option. **Back** returns to the previous page. Each plugin has its own preset and Restore Defaults action; those actions leave the joined Game Settings values unchanged. The game's confirmed **Restore Default Settings** action also resets the added Game Settings values. A restart notice appears when an option needs the game to restart.

| Setting | Values | Default |
|---|---|---|
| Resolution width and height | 0 = desktop resolution, or a size; Hub/settings file; restart | 0 |
| HUD aspect ratio | Auto for full HUD width, or 4:3, 16:9, 21:9/custom up to 32:9; live | Auto |
| FOV multiplier | 0.5–2.0; larger values widen the view up to 150° horizontally; live | 1.0 |
| FPS limit | 0 = unlimited, -1 = desktop refresh, or 30–1000; live | 60 |
| Raw mouse input | On uses raw relative mouse movement; live | On |
| Text language | Auto follows the Windows display language, or choose English, French, German, Italian or Spanish; Hub/settings file | Auto |
| Skip intro | On or Off; restart | On |
| Window mode | Fullscreen, Windowed or Borderless; restart | Fullscreen |
| Pause when unfocused | On pauses while another window is active; live | On |
| Confine cursor | On keeps the cursor inside the active game window; live | On |
| Cutscene borders | Off, Letterbox, Pillarbox or Both; live | Both |
| Animated cutscene borders | On slides the borders like the original; Off shows them at once; live | On |
| Check for updates | Off makes no network request; live | Off |

Vanilla disables the FPS limit, shows the intro, and keeps the original letterbox only. Recommended and Enhanced skip the intro. The game's videos are all 4:3 and keep their aspect. Hub texts are available in English, French, German, Italian and Spanish.

In-engine cutscenes keep their original framing: the full width of the original picture in a 16:9 frame. Narrower screens show the original bars above and below, wider screens get bars at the sides. With the borders off, cutscenes fill the screen with the gameplay view. The FOV multiplier does not change cutscenes.

With **Check for updates** on, a notice appears in the Plugins page when a newer version is available; FusionFixHub opens after the game exits when it is installed in the game folder.

Ctrl+F10 opens the developer overlay with every setting and a Debug › Info page.

Windowed mode centers the exact rendering client size in the monitor work area. At desktop resolution the client covers the monitor, with the frame outside it. Borderless uses the desktop resolution. The cursor is released when the game loses focus or confinement is disabled. HUD sprites retain their native proportions and pixel sizes, with panels anchored to the screen or selected HUD aspect.

The fix’s own text is available in English, French, German, Italian and Spanish. Choose **Text language** in FusionFixHub or the settings file; Auto follows the Windows display language. Reopen the menu after changing it.

**Raw mouse input** uses Windows raw relative movement for the mouse axes while keeping the game’s button and wheel handling. It applies immediately.

## Known limits

- The field-of-view multiplier also affects scripted gameplay cameras outside cutscenes. Portraits and other menu views keep their original framing.
- Window mode changes require a restart; live resizing and Alt+Enter switching are unavailable.
- Anti-aliasing options are unavailable.
- The PC version has no controller support; use the keyboard and mouse.
- 60 FPS is recommended. Higher frame rates may affect game behavior.
- Other executables can use fusionfix.io, FusionFixHub or the settings file when the in-game menu is unavailable.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
