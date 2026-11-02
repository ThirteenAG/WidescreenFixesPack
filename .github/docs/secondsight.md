<div align="center">

<img src="https://thirteenag.github.io/screens/secondsight/main2.jpg" width="760" alt="Second Sight Fusion Fix">

**Second Sight Fusion Fix** corrects resolution handling, aspect ratio and field of view for widescreen displays, adds window modes, a frame rate limit, HUD and cutscene framing options, and skips the startup logos.

[Website](https://thirteenag.github.io/wfp#secondsight) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SecondSight.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — The game's Graphics Options list every resolution the monitor offers (each at its highest refresh rate) instead of five 4:3 modes, and the game starts at the resolution chosen there.
- **Aspect ratio and field of view** — Correct widescreen projection for gameplay, cutscenes and the 3D menus, with the original tuned framing; it follows a resolution change.
- **Menus** — The menus stay in their 4:3 frame in the middle of the screen.
- **Display scaling** — The game gets the full resolution on scaled desktops.

## Options

- **Window mode** — Fullscreen, Windowed or Borderless, then restart. Windowed gives a centred window whose picture area is exactly the chosen resolution; a window as big as the monitor puts its frame outside the screen. Borderless covers the monitor and shows the chosen resolution scaled to it.
- **Frame rate limit** — The monitor's refresh rate (default), off, or a number of frames per second; applies at once.
- **HUD aspect ratio** — Auto keeps the HUD at the screen edges; a ratio such as 16:9 keeps it in a centred box of that shape, the left part at the box's left edge and the right part at its right edge. Applies at once.
- **Cutscene borders** — Letterbox keeps the game's bars above and below cutscenes; Pillarbox keeps the picture at 4:3 between side bars on wider screens; Both (default) does both and shows the original framing; Off fills the screen. Applies at once.
- **Movie framing** — How the logo videos at the start are shown on wider screens: the whole video inside the game's centred 4:3 frame (Original) or fitted to the screen (Fill, default); nothing is cut off.
- **Field of view** — A multiplier (0.5–2.0, default 1.0) on the gameplay camera, limited to 150 degrees horizontally; applies at once. Cutscenes keep their own.
- **Skip intro** — On by default: the game starts at the title screen, past the logo videos and the copyright screen. Needs a restart.
- **Pause in background** — On by default: in a window mode the game pauses while another window is active, as it always has. Off keeps it running.
- **Confine cursor** — Keeps the mouse cursor inside the game window while the game is active.
- **Check for updates** — Off by default. When on, the fix asks fusionfix.io once per start whether a newer version is out; FusionFixHub opens when you close the game.
- **Resolution width / height** — Only in the cfg and FusionFixHub's text editor: override the resolution chosen in the game. 0 uses the game's choice.

---

## Settings and hub

The fix creates `scripts/SecondSight.FusionFix.cfg` on first start; FusionFixHub and fusionfix.io can change it, also while the game runs. Vanilla shows the intro, keeps the game's own cutscene bars, runs at the game's own frame rate and plays the logo videos with black bars; Recommended and Enhanced skip the intro, add side bars in cutscenes, fit the logo videos to the screen and limit the frame rate to the monitor's. Window, resolution and input settings aren't changed by presets. Texts are available in English, French, German, Italian and Spanish.

The settings-language choice in FusionFixHub or the settings file selects the language used by the developer overlay.

## Known limits

- The game's menus are built by its own scripts, so the fix has no page in them: change its settings in FusionFixHub or on fusionfix.io.
- The PC version reads only keyboard and mouse; it has no controller support, so the fix adds none.
- The mouse is read through DirectInput without Windows pointer acceleration, so no raw input option is needed.
- The menu logo may still sit partly above the top edge in some views.
- Wider views (field of view above 1.0) can show through-wall rendering in some locations.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/SecondSight.FusionFix.cfg` with every setting.
4. Pick the resolution in the game's Graphics Options and configure the fix in FusionFixHub or on fusionfix.io. Restart when an option requires it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
