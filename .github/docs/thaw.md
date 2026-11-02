<div align="center">

<img src="https://thirteenag.github.io/screens/thaw/main2.jpg" width="760" alt="Tony Hawk&#x27;s American Wasteland Fusion Fix">

**Tony Hawk's American Wasteland Fusion Fix** corrects resolution handling, aspect ratio, HUD scaling, and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thaw) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksAmericanWasteland.FusionFix/dllmain.cpp)

</div>

---

## Settings

Open **Options → Plugins → Tony Hawk's American Wasteland Fusion Fix** on the main menu (Plugins is the last row), or **Game Options → Plugins** in the pause menu. **HUD aspect ratio** also sits in the game's **Display Options**. `Game/scripts/TonyHawksAmericanWasteland.FusionFix.cfg` is created on first start; fusionfix.io and FusionFixHub change the same settings.

Up and Down choose a row, Left and Right change a value. The selected row's description runs line by line in the help bar at the bottom of the screen. **Preset** applies Vanilla, Recommended or Enhanced to this plugin. A restart notice appears in the help bar when a changed option needs the game to restart, and disappears when the value is set back. **Back** returns to the options menu with Plugins selected.

| Setting | Values | Default |
|---|---|---|
| Resolution width and height | 0 = desktop resolution, or any size, including sizes the monitor doesn't list; FusionFixHub/settings file; restart | 0 |
| Window mode | Fullscreen, Windowed or Borderless; restart | Fullscreen |
| Skip intro | Skips the Aspyr and Activision logos and the intro movie at startup; restart | On |
| Video aspect | Original aspect or Fill; live | Fill |
| HUD aspect ratio | Automatic for the whole screen width, or 4:3, 16:9, 21:9, 32:9 or a custom ratio; live | Automatic |
| Cutscene borders | Off, Top and bottom, Sides or Both; live | Both |
| Pause in background | On stops the game while another window is active, as the original does; Off keeps it running; live | On |
| Field of view | 0.5–2.0; larger values widen the skating camera; live | 1.0 |
| Raw mouse input, mouse sensitivity, invert mouse Y | Mouse camera during gameplay; live | On, 1.0, Off |
| Gamepad camera speed, invert gamepad Y, vibration | live | 1.0, Off, On |
| Gamepad icons | Automatic (Xbox), Xbox, PlayStation or Nintendo; live | Automatic |
| Check for updates | Off makes no network request; live | Off |
| Confine cursor | On keeps the cursor inside the active game window; FusionFixHub/settings file; live | On |

Vanilla shows the logos and the intro movie, shows the movies in the game's 4:3 frame and gives cutscenes the screen width between their bars; Recommended and Enhanced skip the intro, fit the movies to the screen and keep the cutscenes' original picture. The logos and the intro can still be watched in **Options → Movies**. Texts are available in English, French, German, Italian and Spanish; Automatic follows the game's language, and **Text language** in FusionFixHub selects another one.

## What the fix does

- **Any resolution and aspect ratio** from 5:4 to 32:9, with square pixels. The 3D view keeps the original vertical view and shows more at the sides (Hor+). The game copied every frame to the screen with its top and bottom cut off and stretched to the full height; the fix shows the whole frame, so nothing is too tall.
- **HUD:** with **HUD aspect ratio** on Automatic, the score panel with its special meter sits at the left edge of the screen and the current goal at the right edge; trick text, the clock, the balance meter and messages stay centered. A ratio keeps them in a centered box of that shape. In HORSE and two-player split screen the score panels and timers follow the box for each player. Nothing is stretched.
- **Menus and 2D screens** (main menu, options, level select, skater select, pause menu with its map, time and money) keep their 4:3 layout in the middle of the screen. On screens narrower than 4:3, such as 5:4, menus and HUD fit the width with the 3D scene above and below.
- **Story cutscenes** show a wide picture between black bars, as in the original. **Both** keeps exactly that picture in the middle of the screen; **Top and bottom** spans the screen width between the bars; **Sides** fills the screen height up to the picture's shape; **Off** fills the whole screen. Every mode shows the scene undistorted and can be changed while a cutscene plays.
- **Movies** are never cut off or stretched: Original shows them inside the game's centered 4:3 frame, Fill fits them to the screen. The game's movies are 4:3 pictures, so on wide screens both show them at full height with black bars at the sides.
- **Window modes** at any Windows display scaling, without rounded corners: Windowed opens a centered window with exactly the chosen client size, also for sizes the monitor doesn't list, and the window can be resized while playing. The window is not minimized when another window becomes active.
- **Keyboard without the launcher:** when the launcher never saved controls, or left an action such as Pause without a key, the launcher's default key is used for it (Enter pauses).
- **Mouse camera:** raw mouse movement turns the gameplay camera, with its own sensitivity and vertical inversion.
- **Controllers:** the game reads gamepads through DirectInput; the bundled Xidi presents an Xbox controller to it. When no controller is assigned, the first one found at startup gets the default layout; existing assignments and bindings stay. Button prompts switch between keyboard keys and the chosen icon set as you use each device.
- **Soundtrack:** the random song order works.
- **Update notice:** with **Check for updates** on, a new version is announced in the help bar of the Plugins list; FusionFixHub opens after the game exits when it is installed.
- **Ctrl+F10** opens the in-game settings overlay.

**Frame rate limit** defaults to 60 FPS and can be changed live. Higher rates may affect gameplay; 0 removes the limit and -1 follows the monitor refresh rate.

## Known limits

- Alt+Enter isn't handled; the window mode applies at start.
- Turning **Pause in background** off while the game is already paused in the background takes effect when the game window is active again.

## Installation

Extract the release archive into the game folder, preserving its folders; the files belong beside `Game/THAW.exe`. Launch the game, then configure the fix in the game's Options → Plugins, on fusionfix.io or in FusionFixHub.


<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
