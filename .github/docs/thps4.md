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
| HUD aspect ratio | Auto for the whole screen width, or 4:3, 16:9, 21:9, 32:9 or a custom ratio; live | Auto |
| Pause in background | On stops the game while another window is active, as the original does; Off keeps it running; live | On |
| Frame rate limit | 30–60 fps; the game's physics, animation and timers are made for 60 fps; live | 60 |
| Gamepad icons | Auto (Xbox), Xbox, PlayStation or Nintendo button pictures in prompts; live | Auto |
| Check for updates | Off makes no network request; live | Off |
| Field of view | 0.5–2.0; larger values widen the skating camera; live | 1.0 |
| Confine cursor | On keeps the cursor inside the active game window; Hub/settings file; live | On |

Vanilla shows the logos and the intro movie; Recommended and Enhanced skip them. The logos and the intro can still be watched in **Options → Movies**. Texts are available in English, French, German, Italian and Spanish; Automatic uses English, and **Text Language** in FusionFixHub selects another one.

## What the fix does

- **Any resolution and aspect ratio** from 5:4 to 32:9, with square pixels. The 3D view keeps the original vertical view and shows more at the sides (Hor+).
- **HUD:** with **HUD aspect ratio** on Auto, the score, special meter and stance sit at the left edge of the screen, and the goal, cash and pro points at the right edge; trick text, the balance meter and messages stay centered. A ratio keeps them in a centered box of that shape. Nothing is stretched. In side-by-side split screen each player's HUD sits in their own half.
- **Menus and 2D screens** (main menu, options, level select, skater select, dialogs) keep their 4:3 layout in the middle of the screen; the button help bar at the bottom spans the whole width. On screens narrower than 4:3, such as 5:4, they fit the width with the 3D scene above and below.
- **Movies** are never cut off or stretched. All of the game's movies are 4:3 pictures, shown at full height with black bars at the sides.
- **Window modes** at any Windows display scaling, without rounded corners: Windowed opens a centered window with exactly the chosen client size, also for sizes the monitor doesn't list. The window is not minimized when another window becomes active.
- **Soundtrack:** the random song order works.

Controllers: the game reads gamepads through DirectInput; the bundled Xidi presents an Xbox controller to it. Without a controller chosen in the game's launcher, the first controller is player 1's and a second one player 2's, with the console layout: A ollie, X flip, B grab, Y grind, Start pause, shoulder buttons for spins and revert. **Options → Control Setup** still changes the bindings. While the controller is in use, the button help and other prompts show its buttons (pick the set under **Gamepad icons**); a key press brings the keyboard keys back.

## Known limits

- The window size is set at startup (resolution settings); resizing the window while playing isn't supported.
- The game has no letterboxed cutscenes; level intros use the same widened camera as gameplay.

## Installation

Extract the release archive into the game folder, preserving its folders; the files belong beside `Game/Skate4.exe`. Keep `d3d8.dll` and `d3d8.ini` together there. Launch the game, then configure the fix in the game's Options → Plugins, on fusionfix.io or in FusionFixHub.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark&layout=mobile"><source media="(max-width: 768px)" srcset="https://fusionfix.io/credits/supporters.svg?layout=mobile"><source media="(prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark"><img width="100%" src="https://fusionfix.io/credits/supporters.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
