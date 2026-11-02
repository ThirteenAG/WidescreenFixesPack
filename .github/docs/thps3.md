<div align="center">

<img src="https://thirteenag.github.io/screens/thps3/main2.jpg" width="760" alt="Tony Hawk&#x27;s Pro Skater 3 Fusion Fix">

**Tony Hawk's Pro Skater 3 Fusion Fix** corrects resolution handling, aspect ratio, HUD scaling, and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thps3) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksProSkater3.FusionFix/dllmain.cpp)

</div>

---

## Settings

Open **Options → Plugins → Tony Hawk's Pro Skater 3 Fusion Fix** for the fix's settings and presets. Plugins is the last row of the Options menu on the main menu (the in-game pause menu has no options page). `scripts/TonyHawksProSkater3.FusionFix.cfg` is created on first start; fusionfix.io and FusionFixHub change the same settings.

Left and Right change a value, Enter steps it forward. **Details** below a setting shows its full description; **More pages** and **Previous pages** move through the list and **Back** returns. Presets apply only to this plugin. A restart notice appears when a changed option needs the game to restart, and disappears when the value is set back.

| Setting | Values | Default |
|---|---|---|
| Resolution width and height | 0 = desktop resolution, or any size, including sizes the monitor doesn't list; Hub/settings file; restart | 0 |
| Window mode | Fullscreen, Windowed or Borderless; restart | Fullscreen |
| Field of view | 0.5–2.0; larger values widen the skating camera; live | 1.0 |
| HUD aspect ratio | Auto for the whole screen width, or 4:3, 16:9, 21:9, 32:9 or a custom ratio; live | Auto |
| Video mode | Original aspect or Fill; live | Fill |
| Skip intro | Skip the publisher and studio logos and the intro movie at startup; restart | On |
| Confine cursor | On keeps the cursor inside the active game window; Hub/settings file; live | On |
| Check for updates | Off makes no network request; live | Off |

Vanilla shows the logos and the intro movie; Recommended and Enhanced skip them. The intro can still be watched in **Options → Movies**. Texts are available in English, French, German, Italian and Spanish; Automatic uses English, and **Text Language** in FusionFixHub selects another one.

## What the fix does

- **Any resolution and aspect ratio** from 5:4 to 32:9, with square pixels. The 3D view keeps the original vertical view and shows more at the sides (Hor+).
- **HUD:** with **HUD aspect ratio** on Auto, the score and special meter sit at the left edge of the screen and the stance icon at the right edge; the timer, trick text, balance meter and goal messages stay centered. A ratio keeps them in a centered box of that shape. Nothing is stretched.
- **Menus and 2D screens** (main menu, options, level select, park editor, goal lists) keep their 4:3 layout in the middle of the screen. On screens narrower than 4:3, such as 5:4, they fit the width with the 3D scene above and below. The level-select tapes are centered.
- **Movies** are never cut off or stretched. All of the game's movies are 4:3, so Original and Fill both show them at full height with black bars at the sides.
- **Window modes** at any Windows display scaling, without rounded corners: Windowed opens a centered window with exactly the chosen client size, also for sizes the monitor doesn't list. In windowed modes the window is not minimized when another window becomes active; the game keeps running in the background and takes keyboard input again when you return.
- **Soundtrack:** the random song order works.

Controllers: the game reads gamepads through DirectInput; the bundled Xidi presents an Xbox controller to it. Choose it under **Options → Controller Config → Primary Controller** and bind the buttons there.

**Frame rate limit** defaults to 60 FPS and can be changed live. Higher rates may affect gameplay; 0 removes the limit and -1 follows the monitor refresh rate.

## Known limits

- The window size is set at startup (resolution settings); resizing the window while playing isn't supported.
- No Ctrl+F10 developer overlay: the game renders through Direct3D 8, which the overlay doesn't support.
- The game has no letterboxed cutscenes; the level fly-by previews use the same widened camera as gameplay.
- Prompts use the game's own PlayStation button names; controller icon sets are not available.
- The in-game pause menu has no Plugins entry; change settings from the main menu's Options, in FusionFixHub or on fusionfix.io.

## Installation

Extract the release archive into the game folder, beside `Skate3.exe`, preserving its folders, then launch the game. Configure the fix in the game's Options → Plugins, on fusionfix.io or in FusionFixHub.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
