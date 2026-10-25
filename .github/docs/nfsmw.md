<div align="center">

<img src="https://thirteenag.github.io/screens/nfsmw/attract.jpg" width="760" alt="Need for Speed: Most Wanted - Widescreen Fix">

**Need for Speed: Most Wanted Widescreen Fix** adds proper widescreen support, corrects HUD and FOV, shows movies at their original aspect ratio, adds modern gamepad support, adds frame pacing and selected high-frame-rate corrections and fixes shadows.

[Website](https://thirteenag.github.io/wfp#nfsmw) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSMostWanted.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/nfsmw/menu.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen menu">

- **Resolution** - native support for any display resolution, including ultrawide
- **Aspect Ratio** - corrected for all rendered elements; the front end fits screens narrower than 4:3
- **HUD** - repositioned and scaled to match the active aspect ratio, anchored at the screen edges; optional constraint next to the game's video options (4:3, 16:9, 21:9, 32:9, or a custom ratio set on fusionfix.io or in FusionFixHub), applied at once when changed
- **Field of View** - corrected for widescreen, matching the Xbox 360 version; **FOV Scale** also offers the original PC field of view or a mathematically correct hor+ one
- **Movies** - shown at their correct aspect ratio (the 2:1 frames of the game's movies hold a 16:9 picture, which is cropped out); HD replacement movies are shown with square pixels; **Movie Display** chooses Fill Screen (default, cropped to fill the screen), Original Aspect (black bars) or Stretch, and applies at once
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsmw/race2.jpg" width="360" align="right" style="margin: 0 0 0 24px;" alt="Shadows">

- **High frame rates** - the game simulates at the frame rate limit, so it adds frame pacing and selected high-frame-rate corrections; frames are paced precisely, without the judder of the original frame limiter; animated textures and world streaming run at the correct speed above 60 FPS; menu and HUD animations no longer run slow; NOS trails keep their length
- **Timebug** - the simulation time is counted in double precision, so race times and timers no longer drift after long sessions
- **Crash fixes** - a language without a matching `LANGUAGES\*.BIN` file (the default language is now one whose file exists), skipping the intro with missing movie files, the roadblock "pure virtual function call", the front end listener list, the sound thread when the audio device fails, and a missing attribute key name
- **Shadows** - no longer disappear in tunnels and under bridges; improved shadow LOD; resolution scaled with the aspect ratio
- **High Spec Audio** - 44100 Hz audio regardless of registry settings
<br clear="both">

---

## New Options

<img src="https://thirteenag.github.io/screens/nfsmw/drag.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Gameplay">

- **In-Game Options** - display mode, field of view, HUD width and console gamma follow the game's own video options, anti-aliasing and the shadow settings its advanced video options; everything else is under one new **Plugins** category in the Options menu (also in the pause menu). It lists the fix first, then every other loaded plugin with settings (such as Xbox Rain Droplets); each opens its page with its Preset row, its settings and Restore Defaults. Values retain the game's font size; long labels fit beside them. Letters unavailable in the active game font use readable Latin replacements. Selected help appears below the list; long text continues automatically. Video settings whose rows cannot be added remain available under Plugins. The game's Video defaults reset the added settings on that page. A change that needs a restart shows the game's message once. All settings are stored in `NFSMostWanted.WidescreenFix.cfg` next to the plugin, which the fix writes with every setting and a comment above each; settings can also be changed on fusionfix.io or in FusionFixHub. An old `scripts/NFSMostWanted.WidescreenFix.ini` isn't used anymore and can be deleted
- **Presets** - the Preset row at the top of the fix's page picks Vanilla, Recommended or Enhanced for the picture, movie, cutscene, camera, frame rate and intro options (accept twice to apply; Vanilla is the original game with its 60 FPS cap and only the fixes); your window mode, language and control settings stay as they are
- **Language** - switch the game's language on the fix's page (after a restart); only the languages installed with the game are offered, so the game no longer crashes on a language whose files are missing
- **Cutscene Borders** - Pillarbox (and Both, the default) keeps the in-game cutscenes (race intros, pursuit and story scenes) at 16:9 on wider screens with black sides; Off shows them at the full width. The PC version draws no bars above and below in these scenes, so Letterbox shows none
- **Skip Intro** - bypass startup FMVs and the title screen, straight to the game menu
- **Subtitles** - subtitles in the story movies and tutorials in English too; the game only showed them for the other languages
- **Display Mode** - borderless, bordered, resizable, or borderless fullscreen; in every mode the game gets the full resolution on scaled desktops
- **FPS Limit** - synced to the monitor refresh rate, twice the refresh rate, or a fixed value
- **Gamepad Support** - Xbox, PlayStation, Switch and other controllers through [Xidi](https://github.com/samuelgr/Xidi) with the [SDL3 plugin](https://github.com/RibShark/Xidi-SDL3-Plugin): modern default controls with independent analog triggers, controller button names in the controls menu, console button icons in the front end, and gamepad buttons for all front end actions (Xbox or PlayStation icons, detected automatically or chosen in the menu)
- **Stick Dead Zones** - separate dead zones for the left and right stick, in percent (default 10)
- **Field of View** - multiplies the driving camera field of view from 0.5 to 2.0 (default 1.0) and applies while the game runs; native angle limits still apply
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsmw/race3.jpg" width="360" align="right" style="margin: 0 0 0 24px;" alt="Racing">

- **Shadow Resolution** - configurable dynamic shadow resolution (1024 / 2048 / 4096 / 8192 in the menu, up to 16384 on fusionfix.io or in FusionFixHub), with optional soft shadows
- **Custom Save Directory** - redirect user files to any subfolder in the game directory
- **Registry-Free Settings** - optionally saves all game settings to `settings.ini` instead of the registry
- **Rain Droplet Size** - adjusts the size of the on-screen rain droplets
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsmw/race4.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Racing">

- **Console Gamma** - a custom gamma curve that produces a higher-contrast image with deeper colors, or the gamma of the Xbox 360 version of NFS MW; off by default, on in the Enhanced preset
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect
- **AA Artifact Fix** - hides the 1px lines at the screen edges caused by MSAA (top and left, or all sides)
<br clear="both">

---

## Camera

<img src="https://thirteenag.github.io/screens/nfsmw/camera.gif" width="400" align="right" style="margin: 0 0 16px 24px;" alt="Mouse look in-game">

- **Mouse/Stick Look** - free look for the chase cameras with the right stick or the mouse. Like the car camera of GTA San Andreas, the look direction stays fixed in the world while the car turns; after the return delay the camera eases back behind the car. Experience *Need for Speed: Most Wanted* like never before with new camera angles. Free look, stick and mouse sensitivity, invert look, horizontal-only right stick, return delay and return speed are on the fix's page under Plugins.
<br clear="both">

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/nfsmw/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/nfsmw/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |
| <img src="https://thirteenag.github.io/screens/nfsmw/vanilla_stretched_ultra.png" alt="Before stretched ultrawide"> | <img src="https://thirteenag.github.io/screens/nfsmw/vanilla_fixed_ultra.png" alt="After fixed ultrawide"> |
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
2. Extract the contents directly into the game folder - the same folder as `speed.exe`. Keep `Xidi.32.dll`, `SDL.XidiPlugin.32.dll` and `Xidi.ini` next to it for gamepad support (`dinput8.dll` loads Xidi).
3. Remove NFS XtendedInput if it is installed; the fix replaces it.
4. Launch the game once to create `scripts\NFSMostWanted.WidescreenFix.cfg`. The modern gamepad layout is set up the first time a controller is used; **Reset to defaults** in the controller settings restores it.
5. Change the settings in the game's Options menu (video options and Plugins), in FusionFixHub, or on fusionfix.io. Restart the game when an option requires it.

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
| LS / RS | Look back / Pull back camera | |
| LS + RS | | Quit |
| D-pad | Engage event, HUD, steer | Navigate |
| Right stick | Free look | |

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
