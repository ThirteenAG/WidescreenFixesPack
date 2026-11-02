<div align="center">

<img src="https://thirteenag.github.io/screens/thesaboteur/main2.jpg" width="760" alt="The Saboteur Fusion Fix">

**The Saboteur Fusion Fix** corrects widescreen cameras and HUD placement and adds native settings, cinematic and video framing, window options and frame timing fixes.

[Website](https://thirteenag.github.io/wfp#thesaboteur) · [Source code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheSaboteur.FusionFix/dllmain.cpp)

</div>

## Installation

1. Extract the release beside `Saboteur.exe`, preserving its folders. Keep `dinput8.dll`, `FusionFixHub.exe`, the ASI and the accompanying movie and button artwork files together.
2. Configure the fix in FusionFixHub or on fusionfix.io, or start the game and open **Settings → Plugins → The Saboteur Fusion Fix**. The pause menu has the same Plugins entry.
3. The first launch creates `scripts/TheSaboteur.FusionFix.cfg`. Restart when the menu displays a restart notice.

## Cameras and interface

- Widescreen field of view always preserves the gameplay camera's original vertical view and expands horizontally. The normal gameplay FOV multiplier ranges from 0.5 to 2.0, with a 150° horizontal ceiling. Aim, zoom, scripted and special camera controllers retain their intended lens.
- **HUD aspect ratio constraint** moves edge information into a centered area at the chosen maximum aspect. Auto uses the available width. The minimap frame, map and markers move together; menus, subtitles, reticles and full-screen effects keep their own layout.
- **Cutscene borders** preserve the original 16:9 cinematic frame. Letterbox retains the original horizontal artwork and fits the frame on narrower displays; Pillarbox adds side borders on wider displays; Both enables both. Off fills the screen with a view that expands horizontally. The gameplay multiplier leaves cinematic lenses unchanged.
- **Animated cutscene borders** follows the original horizontal-bar animation, or makes those bars immediate when disabled. Scenes without authored bars remain unbarred apart from the borders needed to contain their frame.
- **Movie framing** offers Original, which fits the whole video in a centered 4:3 area, and Fill, which fits it to the screen. Video framing is independent of cinematic borders.

## Other settings

Fullscreen, framed Windowed and Borderless modes are available. Resolution values of 0 use the desktop resolution. Window mode and resolution require a restart; the game's device setup does not provide live resizing or an Alt+Enter transition. Windowed clients render at the selected size, with DPI handling and square corners.

The frame limit accepts 30–1000 FPS, 0 for unlimited, or -1 for the monitor refresh rate. Clock, driving and relative DirectInput mouse timing corrections remain active at every limit. **60 FPS is recommended**; the full mission and physics matrix above 60 FPS has not been exhaustively tested.

Pause in Background and Confine Cursor apply live. Skip Intro Videos affects startup logos and the title trailer; story scenes remain available. Updates are off by default and can be enabled for notices in the native Plugins menu.

Controller icons offer Auto, Xbox, PlayStation and Nintendo artwork using the game's native XInput support and bindings. Auto uses Xbox icons. Icon changes require a restart because the game caches its movie textures.

The existing SMAA, console gamma, map marker, high-resolution interface/font/cursor and shutdown audio fixes are retained. Enhanced adds SMAA and console gamma; Recommended uses the 60 FPS limit. Vanilla keeps the corrections active while retaining intros.

**Ctrl+F10** opens the developer overlay where available. After closing it, gameplay input resumes once held movement inputs return to neutral. **Debug → Info** shows the fix version, UTC build time, graphics API and actual backbuffer size. Native option help, full Details, per-plugin presets and restart notices are also available in the Plugins menu.
