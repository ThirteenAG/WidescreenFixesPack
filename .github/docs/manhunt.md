<div align="center">

<img src="https://thirteenag.github.io/screens/manhunt/main2.jpg" width="760" alt="Manhunt Fusion Fix">

**Manhunt Fusion Fix** adds Hor+ widescreen, keeps the HUD, menus and videos at their intended shape, and adds window modes, a frame rate limit, cutscene framing and native in-game settings.

[Website](https://thirteenag.github.io/wfp#manhunt) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/tree/master/source/Manhunt.FusionFix)

</div>

---

## Fixes

- **Field of view** - the gameplay multiplier is neutral at 1.0 and stops at 150 degrees across. Hor+: the picture keeps the original height and gets wider with the screen; visibility culling follows. Screens narrower than 4:3 see more above and below instead. The game's own "Ratio" option (an anamorphic squeeze for TVs) no longer changes the camera.
- **HUD** - elements keep their 4:3 shape and move to the screen's edges: the radar to the left edge, the player status, health and stamina, inventory and timer to the right edge; text boxes keep their side, centered elements stay centered. A HUD aspect ratio limits this to a centered box of that aspect.
- **Menus and loading screens** - drawn at their 4:3 design in the middle of the screen with black beside the original pictures. Wider pictures (the optional **Manhunt.WidescreenFrontend** package) are shown at their own shape. The menu cursor moves at the same speed in both directions and stays over what it points at.
- **Screen effects** - noise, scanlines and screen tints cover the whole screen; sprites placed from the 3D view (lock-on, light glows, blood, halos) keep their shape; the sniper scope gets black sides across the whole screen.
- **Cutscenes** - Both keeps the original 4:3 shot with side borders on wide screens and top and bottom borders on narrow screens. Off removes those borders and fills the screen. The REC and time overlays stay inside the framed image. The original game has no animated cutscene bars.
- **Videos** - the 512x512 frames are shown whole at 4:3, pillarboxed with black, in both choices.
- **Resolution** - the startup dialog lists every fullscreen mode; on the first start it selects the desktop resolution. Fullscreen modes use the highest refresh rate the monitor offers at that size (the game took the lowest).
- **Window modes** - windowed (at the resolution chosen in the startup dialog, centered; at the monitor's size the frame lies outside the screen) or borderless (covering the monitor). The window has no rounded corners on Windows 11.
- **Cursor** - the mouse cursor stays inside the game's window while the game is in front, so it can't slip onto another monitor; it's let go when you switch away or move the window.
- **Frame rate** - an optional limit that sleeps instead of the game's busy wait. The game never runs faster than its own 200 fps.
- **Focus** - Pause When Unfocused pauses when another window is active. Turning it off lets Windowed and Borderless continue in the background; Fullscreen retains its native pause.
- **Controller** - the included Xidi connects XInput controllers to the game's native controls. Modern provides a fixed layout, camera speed and Xbox, PlayStation or Nintendo artwork. Controls, menu hints and supported tutorials follow the last input device. Keyboard and mouse assignments are preserved; Original retains saved native controller assignments.
- **Updates** - Check for Updates is off by default. When enabled, a new-version notice appears in Settings. FusionFixHub opens after the game exits if it is installed; otherwise the notice points to fusionfix.io.
- **Skip intro** - the three-second wait on the legal screen and the opening logo video are skipped.
- **Stability** - the folder in Documents for saves and settings is found even if its name isn't set up yet.

## Options

Open **Settings → Plugins → Manhunt Fusion Fix** for settings and per-plugin presets. Choose **Details** to read option help. **Settings → Video** also contains Field of View and HUD Aspect Ratio; use Left/Right to change them while playing. The game’s confirmed Default Settings action resets the fix’s options too. Window mode and Skip Intro need a restart.

In FusionFixHub, Text Language follows the game when set to Auto, or you can choose English, French, German, Italian or Spanish. If Plugins is unavailable with your executable, configure the fix in FusionFixHub.

| Setting | Values | Default |
|---|---|---|
| HUD aspect ratio | Auto (whole screen width), 4:3, 16:9, 21:9 or any width:height | Auto |
| Video mode | Original aspect, Fill | Fill |
| Cutscene borders | Off, Letterbox, Pillarbox, Both | Both |
| Window mode | Fullscreen, Windowed, Borderless (restart) | Fullscreen |
| Confine cursor | On or off | On |
| Frame rate limit | Refresh rate, the game's own (200), or 30-200 | 60 |
| Field of view | 0.5-2.0, up to 150 degrees across, gameplay only (not cutscenes, zoomed views or menus) | 1.0 |
| Skip intro | On or off (restart) | On |
| Controller layout | Original, Modern | Modern |
| Controller buttons | Auto (Xbox), Xbox, PlayStation, Nintendo | Auto |
| Controller camera speed | 0.1-5.0 with Modern | 1.0 |
| Vibration | On or off; native attack feedback through Xidi | On |
| Pause when unfocused | On or off; Off requires Windowed or Borderless | On |
| Check for updates | On or off | Off |

The fix creates `scripts/Manhunt.FusionFix.cfg` on the first start; FusionFixHub can change it, also while the game runs.

Modern uses left stick movement, right stick look, RT attack, LT aim, A use, B sneak, X reload and Y inventory swap. Stick clicks run/look back, Back changes view, Start pauses, bumpers peek and D-pad Up/Down cycle inventory. Menus use the left stick or D-pad, A to confirm and B to go back. The native Controls page retains sensitivity and Invert Y. Controller artwork appears in its second binding column while the pad is in use; keyboard input restores the native key labels.

Presets: Vanilla keeps the original border behavior, shows the videos at their original aspect, keeps the game's own frame cap and shows the intro; Recommended and Enhanced frame cutscenes with Both, fit videos to the screen, cap at 60 FPS and skip the intro. Window mode and Confine cursor aren't changed by presets. Texts are available in English, French, German, Italian and Spanish.

Use the Ultimate ASI Loader included with the fix as `dinput8.dll` so the fix loads before the game initializes.

## Known limits

- Resolution and window mode are chosen at startup; there is no live resize or Alt+Enter reset. The startup video-mode dialog uses Windows controls.
- Windowed startup and high-DPI placement after the video-mode dialog need further testing.
- The native engine's 200 fps ceiling is retained. Windowed presentation can impose a lower ceiling; high-frame-rate mission behavior still needs broader testing.
- Raw mouse input replaces relative movement while preserving the game’s button, wheel, sensitivity and inversion handling. Physical mouse behavior needs broader testing.
- Controller-only mission progression, hotplug, stick feel and physical vibration need broader compatibility testing. Modified tutorial catalogs retain their original text when they do not match a supported prompt.
- Ordinary tutorial boxes rewrap when the input device changes. Scrolling and typewriter overlays retain their native layout until the next message.
- The developer overlay is unavailable with the native Direct3D 8 renderer.

---

## Installation

1. Extract the archive into the game folder, beside `manhunt.exe`, keeping its folders.
2. Optionally extract **Manhunt.WidescreenFrontend** too, for widescreen menu backgrounds.
3. Open Settings → Plugins in the game, or change settings on fusionfix.io or in FusionFixHub. Window mode and Skip Intro need a restart.
