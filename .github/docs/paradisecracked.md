<div align="center">

<img src="https://thirteenag.github.io/screens/paradisecracked/main2.jpg" width="760" alt="Paradise Cracked Fusion Fix">

**Paradise Cracked Fusion Fix** runs the game at any resolution and aspect ratio, with the menus and movies kept in shape and window modes for modern Windows.

[Website](https://thirteenag.github.io/wfp#paradisecracked) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ParadiseCracked.FusionFix/dllmain.cpp)

</div>

---

## Settings

`scripts/ParadiseCracked.FusionFix.cfg` is created on first start. Change the settings on fusionfix.io, in FusionFixHub, or in the game with **Ctrl+F10**.

| Setting | Values | Default |
|---|---|---|
| Display mode | Fullscreen, Windowed or Borderless; restart | Fullscreen |
| Resolution width and height | 0 = desktop resolution, or any size from 800x600 up; FusionFixHub/settings file; restart | 0 |
| HUD aspect ratio | Auto for the whole screen width, or 4:3, 16:9, 21:9, 32:9 or a custom ratio; live | Auto |
| Movie framing | Original or Fill; live | Fill |
| Field of view factor | 0.5–2.0; larger values show more of the mission; live | 1.0 |
| Skip intro | Skips the publisher and studio logos and the title movie at startup; restart | On |
| Pause in background | On pauses the game while another window is active, as the original does; Off keeps it running; live | On |
| Confine cursor | Keeps the cursor inside the active game window; live | On |
| Check for updates | Off makes no network request; live | Off |

Vanilla shows the intro and the movies as the game does; Recommended and Enhanced skip the intro and fill the screen with the movies. The title movie can still be watched in the **Video** menu. Texts are available in English, French, German, Italian and Spanish.

## What the fix does

- **Any resolution and aspect ratio** from 5:4 to 32:9 with square pixels. The resolution of the fix replaces the one set in the game's options program; the default is the desktop resolution. The mission view keeps the original vertical view and shows more at the sides.
- **Menus** are the game's 800x600 picture, shown at the full screen height in the middle of the screen with black sides; the mouse cursor and the buttons stay where they are drawn.
- **Mission interface:** with **HUD aspect ratio** on Auto, the squad panel and the weapon panel sit at the left edge, the journal and its button at the right edge, and dialogs stay in the middle, as the game lays them out for any resolution. A ratio keeps the panels in a centred box of that shape. Nothing is stretched.
- **Loading screens** fill the screen height in the middle of the screen.
- **Movies** are never cut off or stretched: Original shows them as the game does, inside its centred 4:3 picture; Fill shows the whole movie as large as the screen allows.
- **Window modes** at any Windows display scaling, without rounded corners: Windowed opens a centred window with exactly the chosen picture size (a window as large as the monitor puts its frame outside the screen), Borderless covers the monitor. The window keeps its size when the game switches between menus, movies and missions, and isn't minimized when another window becomes active.
- **Update notice:** with **Check for updates** on, a new version is announced at the main menu; FusionFixHub opens after the game exits when it is installed.
- **Ctrl+F10** opens the in-game settings overlay.

## Not included

- **In-game settings page:** the game's option screens are fixed layouts loaded from its data; the settings are in the overlay, on fusionfix.io and in FusionFixHub.
- **Controller:** the game is played with the mouse (turn-based tactics) and has no controller support.
- **Cutscene borders:** the story is told in movies and dialog windows; there are no in-engine cutscenes.
- **Frame rate:** the game's own frame limiter (about 66 FPS) stays.

## Known limits

- The interface keeps the game's own pixel size: at very high resolutions, such as 4K, the mission panels are small. A lower resolution makes them larger; in Fullscreen and Borderless the picture is then scaled to the screen.
- The window size is set at startup; resizing the window while playing and Alt+Enter aren't supported.
- With a HUD aspect ratio, the tabs of empty squad slots, which the game keeps behind the left screen edge, stay at the screen edge.
- The in-game options panel and other mission screens (inventory, journal, dialogs) keep their pixel size in the middle of the screen.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside `game.exe`, preserving the folders in the archive.
3. Launch the game; configure the fix with Ctrl+F10, on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.
