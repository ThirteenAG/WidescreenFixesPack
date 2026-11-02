<div align="center">

<img src="https://thirteenag.github.io/screens/thps4/main2.jpg" width="760" alt="Tony Hawk&#x27;s Pro Skater 4 Fusion Fix">

**Tony Hawk's Pro Skater 4 Fusion Fix** corrects resolution handling, aspect ratio, HUD scaling, and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thps4) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksProSkater4.FusionFix/dllmain.cpp)

</div>

---

## Settings

Open **Options → Plugins → Tony Hawk Pro Skater 4 Fusion Fix** for the fix's settings and presets. Plugins is the last row of the Options menu, both on the main menu and in the pause menu. `Game/scripts/TonyHawksProSkater4.FusionFix.cfg` is created on first start; fusionfix.io and FusionFixHub change the same settings.

Left and Right change a value, Enter steps it forward. **Details** below a setting shows its full description; **More pages** and **Previous pages** move through the list and **Back** returns to Options with Plugins selected. Presets apply only to this plugin. A restart notice appears when a changed option needs the game to restart, and disappears when the value is set back.

| Setting | Values | Default |
|---|---|---|
| Resolution width and height | 0 = desktop resolution, or any size, including sizes the monitor doesn't list; Hub/settings file; restart | 0 |
| Window mode | Fullscreen, Windowed or Borderless; live | Fullscreen |
| Skip intro | Skip the Activision, Aspyr and Neversoft logos and the intro movie at startup; restart | On |
| Video mode | Original aspect or Fill; live | Fill |
| HUD aspect ratio | Auto for the whole screen width, or 4:3, 16:9, 21:9, 32:9 or a custom ratio; live | Auto |
| Pause in background | On stops the game while another window is active, as the original does; Off keeps it running; live | On |
| Frame rate limit | 30–1000 fps, Unlimited, or the desktop refresh rate; live | 60 |
| Check for updates | Off makes no network request; live | Off |
| Field of view | 0.5–2.0; larger values widen the skating camera; live | 1.0 |
| Confine cursor | On keeps the cursor inside the active game window; Hub/settings file; live | On |

Vanilla shows the logos and the intro movie and leaves the frame rate to the game; Recommended and Enhanced skip the intro and limit the game to 60 fps. The logos and the intro can still be watched in **Options → Movies**. Texts are available in English, French, German, Italian and Spanish; Automatic uses English, and **Text Language** in FusionFixHub selects another one.

## What the fix does

- **Any resolution and aspect ratio** from 5:4 to 32:9, with square pixels. The 3D view keeps the original vertical view and shows more at the sides (Hor+).
- **HUD:** with **HUD aspect ratio** on Auto, the score, special meter and stance sit at the left edge of the screen, and the goal, cash and pro points at the right edge; trick text, the balance meter and messages stay centered. A ratio keeps them in a centered box of that shape. Nothing is stretched.
- **Menus and 2D screens** (main menu, options, level select, skater select, dialogs) keep their 4:3 layout in the middle of the screen; the button help bar at the bottom spans the whole width. On screens narrower than 4:3, such as 5:4, they fit the width with the 3D scene above and below.
- **Movies** are never cut off or stretched. All of the game's movies are 4:3 pictures, so Original and Fill both show them at full height with black bars at the sides.
- **Window modes** at any Windows display scaling, without rounded corners: Windowed opens a centered window with exactly the chosen client size, also for sizes the monitor doesn't list. The window is not minimized when another window becomes active.
- **Soundtrack:** the random song order works.
- **Ctrl+F10** opens the in-game settings overlay.

Controllers: the game reads gamepads through DirectInput; the bundled Xidi presents an Xbox controller to it. Choose and bind it under **Options → Control Setup**.

## Known limits

- The window size is set at startup (resolution settings); resizing the window while playing isn't supported.
- The game has no letterboxed cutscenes; level intros use the same widened camera as gameplay.
- Prompts use the game's own button names; controller icon sets are not available.
- Split-screen keeps the game's own HUD layout.

## Installation

Extract the release archive into the game folder, preserving its folders; the files belong beside `Game/Skate4.exe`. Keep `d3d8.dll` and `d3d8.ini` together there. Launch the game, then configure the fix in the game's Options → Plugins, on fusionfix.io or in FusionFixHub.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
