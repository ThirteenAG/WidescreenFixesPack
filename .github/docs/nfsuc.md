<div align="center">

<img src="https://thirteenag.github.io/screens/nfsuc/attract.jpg" width="760" alt="Need for Speed: Undercover - Fusion Fix">

**Need for Speed: Undercover Fusion Fix** adds proper widescreen support, corrects HUD and FOV, runs at the correct speed at any refresh rate, and includes a range of visual and quality-of-life improvements.

[Website](https://thirteenag.github.io/wfp#nfsuc) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSUndercover.FusionFix/dllmain.cpp)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/nfsuc/race1.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen menu">

- **Resolution** - every display mode of at least 640x480 is listed in the video options menu; long resolution lists no longer overflow the menu and crash
- **HUD** - repositioned and scaled to match the active aspect ratio; **HUD Aspect Ratio** (Options > Gameplay, after GPS Mini-Map) is Auto (the whole screen, the HUD at its edges), 4:3, 16:9, 21:9, 32:9 or a custom ratio set on fusionfix.io or in FusionFixHub, applied with the HUD's next update; the GPS map background and screen transitions cover the full width
- **Movies** - the 16:9 movies were stretched on screens wider than 16:9; **Movie Display** (Options > Plugins > Fusion Fix > Movies & Cutscenes, applied at once): Original Aspect shows them whole with black bars, Fill Screen (the default) keeps their aspect and crops them to cover the screen, Stretch fills the screen
- **Cutscenes** - the in-engine scenes use the widened field of view like the game. **Cutscene Borders** (Options > Plugins > Fusion Fix > Movies & Cutscenes, applied at once): Letterbox keeps the game's bars above and below; without it the bars are gone and the view is zoomed to what was between them, so nothing they hid is shown; Pillarbox keeps the scenes at 16:9 on wider screens; Both (the default). **Animated Cutscene Borders** off shows the bars at full height at once instead of easing them in and out like the scene does
- **Field of View** - corrected for widescreen; **FOV Scale** chooses between the original field of view and a mathematically correct hor+ one
- **High frame rates** - the game simulates at a multiple of the frame rate limit (at least 60 steps per second) with precisely paced frames, so it runs at the correct speed above 60 FPS instead of too fast; the chase camera's springs and animated textures (signs, screens) follow real time
- **Music Crash Fix** - the interactive music no longer crashes on streams that aren't set up yet, which happened mostly when loading a profile on the 1.1.2.1 executables
- **Settings** - a crash no longer resets the game's settings on the next start
- **Profiles** - load like on the Xbox 360, without the "Load complete" message
<br clear="both">

---

## New Options

<img src="https://thirteenag.github.io/screens/nfsuc/race2.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Gameplay">

- **In-Game Options** - the fix's display, camera, graphics and control settings appear beside the matching game options. **Options > Plugins > Fusion Fix** contains the remaining settings, with the fix first in the plugin list. Large pages are split into categories. All settings are stored in `NFSUndercover.FusionFix.cfg` next to the plugin, with comments explaining each setting; they can also be changed on fusionfix.io or in FusionFixHub.
- **Skip Intro** - bypass startup FMVs and the title screen
- **Presets** - **Options > Plugins > Fusion Fix > Preset** selects Vanilla, Recommended or Enhanced for that plugin. Accept the game's confirmation dialog to apply it. Window mode, language, sensitivities and controls keep their current values. **Restore Defaults** resets the settings on that plugin's page; the game's **Default Settings** also resets the added rows on its graphics pages.
- **Language** - the game's language under **Options > Plugins > Fusion Fix > Gameplay**, after a restart; only languages whose text files are installed are listed, so the game no longer crashes on a missing language
- **Display Mode** - borderless, bordered, resizable, or borderless fullscreen; in every mode the picture is neither cropped nor blurred on scaled desktops
- **FPS Limit** - synced to the monitor refresh rate, twice the refresh rate, or a fixed value
- **Gamepad Support** - sharp Xbox or PlayStation button icons (detected automatically or chosen in the menu), the controller's buttons instead of keyboard keys in the front end, and the Xbox 360 default controls for newer Xbox controllers and other XInput pads, which had no default controls
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsuc/race3.jpg" width="360" align="right" style="margin: 0 0 0 24px;" alt="Shadows">

- **Stick Dead Zones** - separate dead zones for the left and right stick, in percent (default 10)
- **Field of View** - multiplies the driving camera field of view from 0.5 to 2.0 (default 1.0) and applies while the game runs; native angle limits still apply
- **Custom Save Directory** - redirect user files to any subfolder in the game directory
- **Registry-Free Settings** - optionally saves all game settings to `settings.ini` instead of the registry
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsuc/race4.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Gameplay">

- **Shadow Resolution** - resolution of each shadow cascade (1024 / 2048 / 4096 / 8192 in the menu; default 2048)
- **CSM Scale** - configurable cascade shadow map scale and per-cascade draw distances (near / mid / far), on fusionfix.io or in FusionFixHub
- **Bloom Intensity** - adjustable bloom effect intensity, on fusionfix.io or in FusionFixHub
- **Motion Blur** - turn motion blur off without affecting the other World FX
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsuc/race5.jpg" width="360" align="right" style="margin: 0 0 0 24px;" alt="Shadows">

- **Extra Scenery Detail** - shows the last scenery detail step too, for more scenery in the distance (World Detail menu)
- **Precomputed Culling** - turn off the precomputed visibility, which hides some scenery that can be seen (World Detail menu)
- **Thorough Scenery Culling** - checks every scenery section instead of the visibility tree: less missing scenery, but slower (World Detail menu)
- **Console Gamma** - a custom gamma curve that creates a more contrasted image with deeper colors, similar to how GTA IV looked on the Xbox 360 (off by default, on in Enhanced)
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect
- **AA Artifact Fix** - hides the 1px lines at the screen edges caused by MSAA (top and left, or all sides)
<br clear="both">

---

## Camera

<img src="https://thirteenag.github.io/screens/nfsuc/camera.gif" width="400" align="right" style="margin: 0 0 16px 24px;" alt="Mouse look in-game">

- **Mouse/Stick Look** - free look for the chase cameras with the right stick or the mouse. Like the car camera of GTA San Andreas, the look direction stays fixed in the world while the car turns; after the return delay the camera eases back behind the car. Experience *Need for Speed: Undercover* like never before with new camera angles. Free look, stick and mouse sensitivity, invert look, horizontal-only right stick, return delay, return speed and Field of View are under Options > Plugins > Fusion Fix > Camera.
<br clear="both">

---

**Confine cursor** (on fusionfix.io and in FusionFixHub, on by default, applies while the game runs) keeps the pointer inside the game window. Focus loss, dragging, system menus and disabling the setting release it. The cursor stays free while you use other windows.

The driving FOV multiplier keeps the original field of view at 1.0 and adjusts the driving camera. If your game version is incompatible with this option, the camera keeps its original behavior and the other fixes remain available.

**Display Mode** offers Fullscreen, Windowed with borders or desktop Borderless, plus resizable, borderless-at-render-size and stretched modes. Changes apply after a restart. After updating from an older release, choose the mode again.

## Known limits

- The frame-rate setting requires a restart and also selects the simulation rate. Unlimited rendering and separate limits for menus and loading are not supported.
- Mouse camera movement uses the game’s input; raw mouse input is not supported.

---

## Installation

Keep the game executable and installed game data on the same game version. Mixing patch versions can cause missing scenery files and prevent the game from loading.

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `nfs.exe`.
3. Launch the game once to create `scripts\NFSUndercover.FusionFix.cfg`.
4. Change the settings in the game's Options menu under Plugins > Fusion Fix, in FusionFixHub, or on fusionfix.io. Restart the game when an option requires it.

<details>
  <summary> Contributors (click to expand) </summary>

@AeroWidescreen @xan1242

</details>

---

## Notes

Use the executable and data files from the same game version. Mixing a newer executable with older data can crash when loading a profile.

Presets don't change the compatibility workarounds.

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br><a href="https://github.com/sponsors/xan1242"><img src="https://img.shields.io/badge/Sponsor_xan1242_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="292"></a><br></p>
