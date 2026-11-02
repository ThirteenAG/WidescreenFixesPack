<div align="center">

<img src="https://thirteenag.github.io/screens/da4/main2.jpg" width="760" alt="Deer Avenger 4 Fusion Fix">

**Deer Avenger 4 Fusion Fix** corrects the main camera aspect ratio for widescreen displays.

[Website](https://thirteenag.github.io/wfp#da4) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/DeerAvenger4.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Aspect ratio** - Correct widescreen aspect ratio scaling; the cave and the hunting levels show more at the sides (Hor+), also after the window is resized.
- **HUD** - The main gameplay HUD keeps its proportions and sits at the screen edges.
- **Screens** - Loading pictures, the pause menu and the cave's dialogs keep their 4:3 design in the middle of the screen.
- **Movies** - Movies keep their aspect and colours in windowed modes.
- **Large windows** - Windows wider than 2048 pixels render normally.
- **Launcher** - The launcher stays readable at high display scaling and lists 32-bit display modes, so the game starts on systems that no longer offer 16-bit modes.

## Options

- **Gameplay Field of View** - Use 1.0 for the intended view, a larger value for a wider view or a smaller value for a narrower view. Changes apply immediately during gameplay. Aiming, scripted views and the hub keep their own framing.
- **HUD width** - Keep side-anchored HUD elements within the chosen screen shape. Automatic uses the full screen width. Applies immediately.
- **Window Mode** - Choose Fullscreen, Windowed or Borderless, then restart. Windowed modes require support from the selected graphics adapter. The window starts at the selected client resolution and can be resized; a window larger than the desktop extends beyond the screen.
- **Skip Intro** - Skip startup movies while retaining later movies.
- **Confine Cursor** - Keep the pointer inside the active game window; it is released when the game loses focus.
- **Pause on Focus Loss** - Pause gameplay while another window is active. Turn it off to keep gameplay running; sound stays muted until the game regains focus.
- **Video Scaling** - Original fits the whole video inside the original 4:3 frame. Fill (the default) fits it to the screen. Neither mode cuts off or stretches the picture. The game's full-screen videos are 4:3, so the choices can look the same.
- **Gamepad** - XInput controllers appear to the game's native joystick input through Xidi.
- **Check for Updates** - Off by default. When enabled, the fix checks once for a newer release and shows a notice in Plugins.

## Known limits

The Field of View setting applies to the hunting levels only. Exhibition previews and the scope overlay keep their native framing. Some native rendering issues remain visible in widescreen. Accents in added menu text are simplified to match the game font. If video framing is unavailable, playback uses the original fitted view.

## Configuration

Settings open from the Plugins button in the launcher and from the Plugins entry below the in-game pause menu (press Down past Exit Game). Both read "Plugins" in every language. The added settings text is available in English, French, German, Italian and Spanish. Automatic uses English; select Text Language in FusionFixHub to use another language.

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. If Plugins is unavailable in your game version, use FusionFixHub or the generated `.cfg` file.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch once to create `scripts/DeerAvenger4.FusionFix.cfg`, then configure the fix in FusionFixHub or on fusionfix.io.
