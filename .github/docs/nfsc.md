<div align="center">

<img src="https://thirteenag.github.io/screens/nfsc/attract.jpg" width="760" alt="Need for Speed: Carbon - Widescreen Fix">

**Need for Speed: Carbon Widescreen Fix** adds proper widescreen support, corrects HUD and FOV, shows movies at their original aspect ratio, adds modern gamepad support, adds frame pacing and selected high-frame-rate corrections and matches the lighting and effects of the Xbox 360 version.

[Website](https://thirteenag.github.io/wfp#nfsc) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSCarbon.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/nfsc/menu.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen menu">

- **Resolution** - native support for any display resolution, including ultrawide
- **Aspect Ratio** - corrected for all rendered elements; the front end fits screens narrower than 4:3
- **HUD** - repositioned and scaled to match the active aspect ratio, anchored at the screen edges; optional constraint in Plugins (4:3, 16:9, 21:9, 32:9, or a custom ratio set on fusionfix.io or in FusionFixHub), applied at once when changed
- **Field of View** - corrected for widescreen, matching the Xbox 360 version; **FOV Scale** also offers the original PC field of view or a mathematically correct hor+ one
- **Movies** - shown at their correct aspect ratio (the 2:1 frames of the game's movies hold a 16:9 picture, which is cropped out); HD replacement movies are shown with square pixels; **Movie Display** chooses Fill Screen (default, cropped to fill the screen), Original Aspect (black bars) or Stretch, and applies at once; fixes movies that started silent and froze
- **Lighting** - Console Lighting uses the Xbox 360 lighting, including the rear-view mirror; off keeps the original PC lighting. Vanilla turns it off; changing it requires a restart
- **Car Shadows** - opacity reduced to match the Xbox 360 version
- **Xenon Effects** - fixed alpha blending state for sparks and contrails
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsc/race2.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Gameplay">

- **High frame rates** - the game simulates at the frame rate limit with precisely paced frames, so it adds frame pacing and selected high-frame-rate corrections; contrails, sparks and NOS trails look the same at any frame rate, with the effect rate limits working for every car, not just one
- **Crash Fix** - saved progress is no longer lost after loading a profile, which was often followed by a crash
<br clear="both">

---

## New Options

<img src="https://thirteenag.github.io/screens/nfsc/race3.jpg" width="360" align="right" style="margin: 0 0 0 24px;" alt="Visual effects">

- **In-Game Options** - window mode follows resolution, picture and movie options follow brightness, FOV follows the car camera, and HUD width follows the interface options. The remaining settings are under one **Plugins** category in the Options menu, also available while paused. It lists the fix first, then other loaded plugins with settings; each opens its own pages, presets and Restore Defaults. Back returns to the plugin list. Extra settings remain available in Plugins. The game's defaults reset the added controls on the current page. Settings are stored in `NFSCarbon.WidescreenFix.cfg` next to the plugin and can also be changed on fusionfix.io or in FusionFixHub. An old `scripts/NFSCarbon.WidescreenFix.ini` isn't used anymore and can be deleted
- **Menu Text** - added settings keep the native font size and complete values. Long plugin values continue on timed pages. Select a setting to read its full name, value and description below the list; longer help continues every 12 seconds. A separate notice appears after the first change that needs a restart
- **Presets** - the fix's Plugins page offers Vanilla, Recommended and Enhanced (accept twice to apply). Vanilla keeps the original presentation and 60 Hz simulation rate with bug fixes; Recommended and Enhanced enable the corresponding improvements. Window mode, language and control preferences stay as they are
- **Language** - the game's language in Plugins (Gameplay), after a restart; only languages whose text files are installed are listed
- **Cutscene Borders** - Pillarbox (and Both, the default) keeps the in-game cutscenes (race intros, canyon duel intros and other scenes) at 16:9 on wider screens with black sides; Off shows them at the full width. The PC version draws no bars above and below in these scenes, so Letterbox shows none
- **Skip Intro** - bypass the startup movies (EA logo, PSA, attract movie) and the title screen, straight to the game menu; other movies still play
- **Display Mode** - borderless, bordered, resizable, or borderless fullscreen; in every mode the game gets the full resolution on scaled desktops
- **FPS Limit** - synced to the monitor refresh rate, twice the refresh rate, or a fixed value
- **Gamepad Support** - Xbox, PlayStation, Switch and other controllers through [Xidi](https://github.com/samuelgr/Xidi) with the [SDL3 plugin](https://github.com/RibShark/Xidi-SDL3-Plugin): modern default controls with independent analog triggers, controller button names in the controls menu, console button icons in the front end, and gamepad buttons for all front end actions (Xbox or PlayStation icons, detected automatically or chosen in the menu); LS + RS quits only from the front end
- **Stick Dead Zones** - separate dead zones for the left and right stick, in percent (default 10)
- **Field of View** - multiplies the driving camera field of view from 0.5 to 2.0 (default 1.0) and applies while the game runs; native angle limits still apply
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsc/race4.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Racing">

- **Custom Save Directory** - redirect user files to any subfolder in the game directory
- **Registry-Free Settings** - optionally saves all game settings to `settings.ini` instead of the registry
- **Motion Blur** - turn motion blur off without losing other effects such as screen flashes or light trails
- **Contrails** - turn off the wind trails behind fast cars
- **Rain Droplet Size** - adjusts the size of the on-screen rain droplets
- **Console Gamma** - a custom gamma curve that creates a more contrasted image with deeper colors, similar to how GTA IV looked on the Xbox 360; off by default, on in the Enhanced preset
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect
- **AA Artifact Fix** - hides the 1px lines at the screen edges caused by MSAA (top and left, or all sides)
<br clear="both">

---

## Camera

<img src="https://thirteenag.github.io/screens/nfsc/camera.gif" width="400" align="right" style="margin: 0 0 16px 24px;" alt="Mouse look in-game">

- **Mouse/Stick Look** - free look for the chase cameras with the right stick or the mouse. Like the car camera of GTA San Andreas, the look direction stays fixed in the world while the car turns; after the return delay the camera eases back behind the car. Experience *Need for Speed: Carbon* like never before with new camera angles. Free look, stick and mouse sensitivity, invert look, horizontal-only right stick, return delay and return speed are in Plugins.
<br clear="both">

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/nfsc/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/nfsc/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |
| <img src="https://thirteenag.github.io/screens/nfsc/vanilla_stretched_ultra.png" alt="Before stretched ultrawide"> | <img src="https://thirteenag.github.io/screens/nfsc/vanilla_fixed_ultra.png" alt="After fixed ultrawide"> |
| The game would also be stretched in ultrawide | Widescreen fix makes it work with any aspect ratio |

</div>

---

**Confine cursor** (on fusionfix.io and in FusionFixHub, on by default, applies while the game runs) keeps the pointer inside the game window. Focus loss, dragging, system menus and disabling the setting release it. The cursor stays free while you use other windows.

The driving FOV multiplier keeps the original field of view at 1.0 and adjusts the driving camera. If your game version is incompatible with this option, the camera keeps its original behavior and the other fixes remain available.

The added in-game settings use English when Russian is selected.

**Display Mode** offers Fullscreen, Windowed with borders or desktop Borderless, plus resizable, borderless-at-render-size and stretched modes. Changes apply after a restart. After updating from an older release, choose the mode again.

## Known limits

- The frame-rate setting requires a restart and also selects the simulation rate. Unlimited rendering and separate limits for menus and loading are not supported.
- Mouse camera movement uses the game’s input; raw mouse input is not supported.

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `NFSC.exe`. Keep `Xidi.32.dll`, `SDL.XidiPlugin.32.dll` and `Xidi.ini` next to it for gamepad support (`dinput8.dll` loads Xidi).
3. Remove NFS XtendedInput if it is installed; the fix replaces it.
4. Launch the game once to create `scripts\NFSCarbon.WidescreenFix.cfg`. The modern gamepad layout is set up the first time a controller is used; **Reset to defaults** in the controller settings restores it.
5. Change the settings in the game's Options menu under Plugins, in FusionFixHub, or on fusionfix.io. Restart the game when an option requires it.

| Gamepad | Race / free roam | Front end |
|:---|:---|:---|
| Left stick | Steer | Navigate |
| RT / LT | Accelerate / Brake, reverse | |
| A | E-brake | Select |
| B | Nitrous | Back |
| X | Speedbreaker | On-screen button |
| Y | Change camera | On-screen button |
| RB / LB | Shift up / down | Next / previous |
| View | Reset car | On-screen button |
| Menu | Pause | Start |
| LS / RS | Look back / Crew ability | |
| LS + RS | | Quit |
| D-pad | Engage event, steer | Navigate |
| Right stick | Free look | Rotate the car |

<details>
  <summary> Contributors (click to expand) </summary>

@AeroWidescreen @xan1242

</details>

---

## Notes

FusionFixHub can change the settings while the game runs. Menu Text Language is on fusionfix.io and in FusionFixHub, on the Compatibility page; Automatic follows the game language. Presets don't change the compatibility settings.

Anti-aliasing offers Off, SMAA, TAA, DLAA and FSR. TAA, DLAA and FSR need the game's own multisampling off. DLAA and FSR need DXVK and the files from FusionFixHub's Anti-Aliasing page; they aren't part of the fix's archive.

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br><a href="https://github.com/sponsors/xan1242"><img src="https://img.shields.io/badge/Sponsor_xan1242_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="292"></a><br></p>
