# Midnight Club LA Remix Fusion Fix (PPSSPP)

[Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/MidnightClubLARemix.PPSSPP.FusionFix/main.cpp)

Widens the 3D view without stretching, keeps the race HUD at the screen edges, keeps menus, loading screens and movies in proportion, and adds a right-stick chase camera and a settings page in the pause menu.

## Features

- **Camera and FOV** — Wider screens keep the original vertical view and show more at the sides; 4:3 shows a little less at the sides. FOV factor scales the 3D view from 0.5 to 2.0.
- **HUD** — In Auto the minimap, speedometer, timer, lap counter, position and other race HUD parts sit at the real screen edges with their original proportions. A 4:3, 16:9, 21:9 or 32:9 HUD box keeps them in a centered area. HUD scale resizes the HUD around its edge anchors (0.5 to 1.5).
- **Menus and loading screens** — Menus, the pause menu, loading screens and prompts keep their original proportions, centered. Loading screens get black bars at the sides instead of the last 3D frame.
- **Race opening fly-by** — Cutscene borders keep the fly-by camera framed like the original: Pillarbox adds side bars on screens wider than 16:9, Letterbox adds bars above and below on narrower screens, Both does both. Off fills the screen with a wider view.
- **Movies** — The attract and intro movies are shown whole, with bars where the screen shape differs. They are made for the PSP screen and have no black bars of their own, so there is nothing to zoom past and no separate movie setting.
- **Dual analog camera** — The right stick turns the chase camera: sideways looks left and right, pulling back looks behind. The D-pad look buttons keep working.
- **Skip intro** — Skips the legal, Rockstar and opening videos and the startup notice that only has an OK button.

## Installation

1. Extract the fix archive into the folder that contains PPSSPP's `memstick` folder, keeping its folders.
2. Enable plugins in PPSSPP and keep the supplied `plugin.ini` next to the plugin.
3. Start the game once to create `memstick/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.cfg`.
4. Change settings in the pause menu, in FusionFixHub, on the website or in the `.cfg` file.

## In-game settings

During a race or cruise, press Start and choose **Plugins**. The first row opens the settings, **Help** explains every setting. Cross changes the selected value (values cycle), **Back** returns.

FOV factor, HUD aspect ratio, HUD scale and cutscene borders apply immediately in the Plugins menu. External changes to those settings and the forced aspect ratio are read while playing when the Plugins page is closed. Skip intro and the dual analog camera require a restart.

**Vanilla** keeps the intro and the original D-pad-only camera. **Recommended** and **Enhanced** skip the intro and turn on the right-stick camera. Other settings keep your values.

The pages follow the PSP system language (English, French, German, Italian or Spanish); `TextLanguage` in the `.cfg` file or the Hub can choose one.

## PPSSPP settings

For Auto, set the display layout you want in PPSSPP (stretched to the window or screen); the fix follows its shape. Aspect ratio in the Hub or `.cfg` can force a `W:H` value instead. Resolution, anti-aliasing, window size and controller bindings are PPSSPP's settings. Bind the right analog stick in PPSSPP's control mapping to use the camera.

## Known limits

- The game runs at its original frame rate.
- Cutscene borders apply to the race opening fly-by; other scripted scenes use the widened view.
- Button prompts keep the PSP artwork.
