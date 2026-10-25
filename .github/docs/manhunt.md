<div align="center">

<img src="https://thirteenag.github.io/screens/manhunt/main2.jpg" width="760" alt="Manhunt Widescreen Fix">

**Manhunt Widescreen Fix** adds Hor+ widescreen, keeps the HUD, menus and videos at their intended shape, and adds window modes, a frame rate limit and a field of view setting.

[Website](https://thirteenag.github.io/wfp#manhunt) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/tree/master/source/Manhunt.WidescreenFix)

</div>

---

## Fixes

- **Field of view** - Hor+: the picture keeps the original height and gets wider with the screen; visibility culling follows. Screens narrower than 4:3 see more above and below instead. The game's own "Ratio" option (an anamorphic squeeze for TVs) no longer changes the camera.
- **HUD** - elements keep their 4:3 shape and move to the screen's edges: the radar to the left edge, the player status, health and stamina, inventory and timer to the right edge; text boxes keep their side, centered elements stay centered. A HUD aspect ratio limits this to a centered box of that aspect.
- **Menus and loading screens** - drawn at their 4:3 design in the middle of the screen with black beside the original pictures. Wider pictures (the optional **Manhunt.WidescreenFrontend** package) are shown at their own shape. The menu cursor moves at the same speed in both directions and stays over what it points at.
- **Screen effects** - noise, scanlines and screen tints cover the whole screen; sprites placed from the 3D view (lock-on, light glows, blood, halos) keep their shape; the sniper scope gets black sides across the whole screen.
- **Videos** - the 512x512 frames are shown at 4:3: filling the screen (cropped), pillarboxed with black, or stretched.
- **Resolution** - the startup dialog lists every fullscreen mode; on the first start it selects the desktop resolution. Fullscreen modes use the highest refresh rate the monitor offers at that size (the game took the lowest).
- **Window modes** - windowed (at the resolution chosen in the startup dialog, centered; at the monitor's size the frame lies outside the screen) or borderless (covering the monitor). The game is sharp on high-DPI displays and its window has no rounded corners on Windows 11.
- **Cursor** - the mouse cursor stays inside the game's window while the game is in front, so it can't slip onto another monitor; it's let go when you switch away or move the window.
- **Frame rate** - an optional limit that sleeps instead of the game's busy wait. The game never runs faster than its own 200 fps.
- **Skip intro** - the three-second wait on the legal screen and the opening logo video are skipped.
- **Stability** - the folder in Documents for saves and settings is found even if its name isn't set up yet.

## Options

Open **Settings → Plugins → Manhunt Widescreen Fix** for settings and per-plugin presets. Choose **Details** to read option help. **Settings → Video** also contains Field of View and HUD Aspect Ratio; use Left/Right to change them while playing. The game’s confirmed Default Settings action resets the fix’s options too. Window mode and Skip Intro need a restart.

Text Language follows the game when set to Auto, or you can choose English, French, German, Italian or Spanish. If Plugins is unavailable with your executable, configure the fix in FusionFixHub.

| Setting | Values | Default |
|---|---|---|
| HUD aspect ratio | Auto (whole screen width), 4:3, 16:9, 21:9 or any width:height | Auto |
| Video mode | Original aspect, Fill, Stretch | Fill |
| Window mode | Fullscreen, Windowed, Borderless (restart) | Fullscreen |
| Confine cursor | On or off | On |
| Frame rate limit | Refresh rate, the game's own (200), or 30-200 | Refresh rate |
| Field of view | 0.5-2.0, gameplay only (not cutscenes, zoomed views or menus) | 1.0 |
| Skip intro | On or off (restart) | On |

The fix creates `scripts/Manhunt.WidescreenFix.cfg` on the first start; FusionFixHub can change it, also while the game runs. An old `scripts/Manhunt.WidescreenFix.ini` isn't used anymore and can be deleted.

Presets: Vanilla shows the videos at their original aspect, keeps the game's own frame cap and shows the intro; Recommended and Enhanced fill the screen with videos, cap at the refresh rate and skip the intro. Window mode and Confine cursor aren't changed by presets. Texts are available in English, French, German, Italian and Spanish.

The fix needs the Ultimate ASI Loader that comes with it (`dinput8.dll`). Installed as `ddraw.dll` (older installs), the loader starts the fix after the game has set itself up; the fix then shows a message and the game runs without it.

## Not covered

- **Cutscene borders** - cutscenes use Hor+ like gameplay. Selectable borders and protection against seeing beyond cutscene sets are not supported.
- **Resolution change while running** - the game sets its video mode once, in the startup dialog.
- **Mouse** - the game reads the mouse through DirectInput. Raw mouse input is not supported.
- **Gamepad** - the game supports DirectInput pads. XInput support and controller icons are not included.

---

## Installation

1. Extract the archive into the game folder, beside `manhunt.exe`, keeping its folders.
2. Optionally extract **Manhunt.WidescreenFrontend** too, for widescreen menu backgrounds.
3. Open Settings → Plugins in the game, or change settings on fusionfix.io or in FusionFixHub. Window mode and Skip Intro need a restart.
