<div align="center">

<img src="https://thirteenag.github.io/screens/thug/main2.jpg" width="760" alt="Tony Hawk&#x27;s Underground Fusion Fix">

**Tony Hawk's Underground Fusion Fix** corrects resolution handling, aspect ratio, HUD scaling, and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thug) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TonyHawksUnderground.FusionFix/dllmain.cpp)

</div>

---

## Settings

Open **Options → Plugins → Tony Hawk's Underground Fusion Fix** for the fix's settings and presets. Plugins is a row of the Options menu, both on the main menu (above Done) and in the pause menu (below Display Options). `Game/scripts/TonyHawksUnderground.FusionFix.cfg` is created on first start; fusionfix.io and FusionFixHub change the same settings.

Up and Down choose a row, Left and Right change a value, Enter steps it forward. The selected row's description runs line by line under the menu, as the game's own menu descriptions do. **Preset** applies Vanilla, Recommended or Enhanced to this plugin; **Restore defaults** resets it. A restart notice appears when a changed option needs the game to restart, and disappears when the value is set back. **Back** returns to Options with Plugins selected.

| Setting | Values | Default |
|---|---|---|
| Resolution width and height | 0 = desktop resolution, or any size, including sizes the monitor doesn't list; FusionFixHub/settings file; restart | 0 |
| Window mode | Fullscreen, Windowed or Borderless; restart | Fullscreen |
| Skip intro | Skips the Activision, Neversoft and Beenox logos and the intro movie at startup; restart | On |
| Video aspect | Original aspect or Fill; live | Fill |
| HUD aspect ratio | Automatic for the whole screen width, or 4:3, 16:9, 21:9, 32:9 or a custom ratio; live | Automatic |
| Cutscene borders | None, Top/bottom, Sides or Both; live | Both |
| Pause in background | On stops the game while another window is active, as the original does; Off keeps it running; live | On |
| Field of view | 0.5–2.0; larger values widen the skating camera; live | 1.0 |
| Check for updates | Off makes no network request; live | Off |
| Confine cursor | On keeps the cursor inside the active game window; FusionFixHub/settings file; live | On |

Vanilla shows the logos and the intro movie, shows the movies in the game's 4:3 frame and gives cutscenes the screen width between their bars; Recommended and Enhanced skip the intro, fill the screen with the movies and keep the cutscenes' original 16:9 picture. The logos and the intro can still be watched in **Options → THUG Movies**. Texts are available in English, French, German, Italian and Spanish; Automatic follows the game's language, and **Text language** in FusionFixHub selects another one.

## What the fix does

- **Any resolution and aspect ratio** from 5:4 to 32:9, with square pixels. The 3D view keeps the original vertical view and shows more at the sides (Hor+).
- **HUD:** with **HUD aspect ratio** on Automatic, the score and special meter (with the stance) sit at the left edge of the screen, and the goals, cash and goal points at the right edge; trick text, the balance meter and messages stay centered. A ratio keeps them in a centered box of that shape. Nothing is stretched.
- **Menus and 2D screens** (main menu, options, level select, skater select, dialogs) keep their 4:3 layout in the middle of the screen; the button help bar at the bottom spans the whole width. On screens narrower than 4:3, such as 5:4, they fit the width with the 3D scene above and below.
- **Story cutscenes** are a 16:9 picture between black bars, as in the original. **Both** keeps exactly that picture in the middle of the screen; **Top/bottom** spans the screen width between the bars; **Sides** fills the screen height up to 16:9; **None** fills the whole screen. Every mode shows the scene undistorted and can be changed while a cutscene plays.
- **Movies** are never cut off or stretched: Original shows them inside the game's centered 4:3 frame, Fill fits them to the screen. The game's movies are 4:3 pictures, so on wide screens both show them at full height with black bars at the sides.
- **Window modes** at any Windows display scaling, without rounded corners: Windowed opens a centered window with exactly the chosen client size, also for sizes the monitor doesn't list. The window is not minimized when another window becomes active.
- **Keyboard without the launcher:** when the launcher never saved controls, the launcher's default keys are used.
- **Soundtrack:** the random song order works.
- **Update notice:** with **Check for updates** on, a new version is announced at the top of the Plugins list; FusionFixHub opens after the game exits when it is installed.
- **Ctrl+F10** opens the in-game settings overlay.

Controllers: the game reads gamepads through DirectInput; the bundled Xidi presents an Xbox controller to it. Choose and bind it under **Options → Control Setup**.

**Frame rate limit** defaults to 60 FPS and can be changed live. Higher rates may affect gameplay; 0 removes the limit and -1 follows the monitor refresh rate.

**Raw mouse input** enables a gameplay mouse camera, with live sensitivity and vertical inversion settings.

## Known limits

- The window size is set at startup (resolution settings); resizing the window while playing and Alt+Enter aren't supported.
- Turning **Pause in background** off while the game is already paused in the background takes effect when the game window is active again.
- Prompts use the game's own button names; controller icon sets are not available.
- Split screen keeps the game's own HUD layout.
- Mouse-camera behavior, including physical input and controller switching, needs gameplay testing.
- The fix supports the game's DRM-free executable (the `THUG.exe` of current releases).

## Installation

Extract the release archive into the game folder, preserving its folders; the files belong beside `Game/THUG.exe`. Launch the game, then configure the fix in the game's Options → Plugins, on fusionfix.io or in FusionFixHub.


<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
