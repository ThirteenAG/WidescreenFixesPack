<div align="center">

<img src="https://thirteenag.github.io/screens/nfsu/attract.jpg" width="760" alt="Need for Speed: Underground - Widescreen Fix">

**Need for Speed: Underground Widescreen Fix** adds proper widescreen support, corrects HUD and FOV, shows movies at their original aspect ratio, adds modern gamepad support and fixes high frame rate issues.

[Website](https://thirteenag.github.io/wfp#nfsu) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSUnderground.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/nfsu/menu.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen gameplay">

- **Resolution** - native support for any display resolution, including ultrawide
- **Aspect Ratio** - corrected for all rendered elements
- **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (16:9, 21:9, or custom), applied at once (the minimap and lap counter follow at the next race)
- **Field of View** - corrected for widescreen; **FOV Scale** chooses between the Xbox version's field of view and a mathematically correct hor+ one
- **Movies** - **Movie Display** shows full screen movies at their original aspect ratio with black bars, filling the screen (cropped, the default) or stretched; the game's anamorphic movies are shown at their intended aspect (the story movies are 16:9, the tutorials 4:3) and HD replacement movies with square pixels; small movies in menus keep their box
- **Cutscenes** - the race start flybys and drag intro shots are in-engine camera shots without black bars, so there is no Cutscene Borders option; they use the player view's hor+ projection like the races
- **High frame rates** - the game runs at the correct speed above 60 FPS; race timers, tick-based gameplay, particles and audio follow real time
- **Controls menu stutter** - the game no longer scans all controllers twice a second while the controls menu is open
- **Debug objects** - development tags only visible in widescreen are hidden
- **Fix** for the **black** background in **magazine cover** screens
- **Fix** for sticky steering, where the opposite direction was ignored until the wheels were centred
- **Audio** - 44.1 kHz output instead of 22 kHz
- **Stability** - fixes the random "0xAAAAAAAA" crash on multi-core CPUs at its cause (the game's memory allocator wasn't thread safe), so the game no longer needs to run on a single CPU core
- **Unsupported versions** - game versions other than 1.4 (such as the original 1.0) show a message and start without the fix instead of crashing
<br clear="both">

---

## Notes

Plugin lists and setting pages use the game's native row counts; longer lists are paged. Two rows below each plugin page show the full selected setting label, value and description. Changes that need a restart add separate warning rows, keeping the notice visible beside the help. Long notes continue on timed pages. Labels fit beside the value column at the original font size.

Added Display-category settings show their full label, value and description below the native rows, using one line in the main menu and two in pause. Long text continues on timed pages. Extra settings remain available in Plugins. Other plugins' labels retain their own case. Latin letters outside the original font use basic Latin replacements. Russian menu text needs game fonts with Cyrillic support. HUD Aspect Ratio affects screens wider than 16:9.

TAA, DLAA and FSR are on the Anti-Aliasing page of the fix in the Plugins menu (main and pause menus), with a graphics API choice and sharpness. They need the game's own multisampling off. DLAA and FSR need DXVK and the files from FusionFixHub's Anti-Aliasing page; they aren't part of the fix's archive.

Menu Text Language is on fusionfix.io and in FusionFixHub, on the Compatibility page; it follows the game language by default and presets don't change it.

## New Options

<img src="https://thirteenag.github.io/screens/nfsu/drag.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Resolution selector in the menu">

- **In-Game Options** - one new **Plugins** entry in the Options menu and in the pause menu's options lists the Widescreen Fix first, then every other plugin with settings (such as Xbox Rain Droplets), and in the main menu the game's credits. Each plugin's page has a **Preset** row (when the plugin has presets; accept asks to confirm), its settings, and **Restore Defaults** (resets only that page, accept twice); Back returns to the list. HUD Aspect Ratio, Field of View, FOV Scale, Console Gamma and Movie Display are in the game's own **Display** options, below the game's rows. When a changed setting applies after a restart, the menu shows the restart warning below the rows. All settings are stored in `NFSUnderground.WidescreenFix.cfg` next to the plugin, which the fix writes with every setting and a comment above each; settings can also be changed on fusionfix.io or in FusionFixHub. An old `scripts/NFSUnderground.WidescreenFix.ini` isn't used anymore and can be deleted
- **Skip Intro** - bypass startup FMVs and the title screen, straight to the game menu
- **Presets** - Vanilla, Recommended (the defaults) and Enhanced presets in the Preset row of the fix's page set the picture, anti-aliasing, movie, camera, frame rate limit and intro options at once (accept asks to confirm); Vanilla uses the game's own 60 FPS cap, no anti-aliasing and original-aspect movies; Recommended adds SMAA; Enhanced adds TAA and console gamma; window mode, language, graphics API, sensitivities and dead zones stay as they are
- **Language** - the game's language, chosen from the languages whose text and speech files are installed; applies after a restart
- **Display Mode** - borderless, bordered, resizable, or borderless fullscreen; in every mode the picture is neither cropped nor blurred on scaled desktops
- **FPS Limit** - uncapped, fixed value, or synced to monitor refresh rate (including 2×)
- **Gamepad Support** - Xbox, PlayStation, Switch and other controllers through [Xidi](https://github.com/samuelgr/Xidi) with the [SDL3 plugin](https://github.com/RibShark/Xidi-SDL3-Plugin): modern default controls with analog triggers, controller button names in the controls menu, gamepad buttons for all front end actions, and button icons that follow the last device used (Xbox or PlayStation style, detected automatically or chosen in the menu); the PC menu bar's quit, delete, default/stock, tutorial, performance and customize actions are on pad buttons (see the table below)
- **Stick Dead Zones** - separate dead zones for the left and right stick, in percent (default 10)
- **Field of View** - multiplies the driving camera field of view from 0.5 to 2.0 (default 1.0) and applies while the game runs; native angle limits still apply

<br clear="both">
<img src="https://thirteenag.github.io/screens/nfsu/drift.jpg" width="360" align="right" style="margin: 0 24px 0px 0;" alt="Resolution selector in the menu">

- **Custom Save Directory** - redirect user files to any subfolder in game directory
- **Console Gamma** - a custom gamma curve that creates a more contrasted image with deeper colors, similar to how GTA IV looked on the Xbox 360; off by default, on in the Enhanced preset
- **Anti-Aliasing** - SMAA (the default), TAA, DLAA or FSR (DLAA and FSR need the files from FusionFixHub's Anti-Aliasing page)
- **AA Artifact Fix** - hides the 1px lines at the screen edges caused by MSAA (top and left, or all sides)
<br clear="both">

---

## Camera

<img src="https://thirteenag.github.io/screens/nfsu/camera.gif" width="400" align="right" style="margin: 0 0 16px 24px;" alt="Mouse look in-game">

- **Mouse/Stick Look** - free look for the chase cameras with the right stick or the mouse. Like the car camera of GTA San Andreas, the look direction stays fixed in the world while the car turns; after the return delay the camera eases back behind the car. Experience **Need for Speed: Underground** like never before with new camera angles. Free look, stick and mouse sensitivity, invert look, horizontal-only right stick, return delay and return speed are on the Camera page of the fix in the Plugins menu.
<br clear="both">

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/nfsu/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/nfsu/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |
| <img src="https://thirteenag.github.io/screens/nfsu/vanilla_stretched_ultra.png" alt="Before stretched ultrawide"> | <img src="https://thirteenag.github.io/screens/nfsu/vanilla_fixed_ultra.png" alt="After fixed ultrawide"> |
| The game would also be stretched in ultrawide | Widescreen fix makes it work with any aspect ratio |

</div>

---

**Confine cursor** (on fusionfix.io and in FusionFixHub, on by default, applies while the game runs) keeps the pointer inside the game window. Focus loss, dragging, system menus and disabling the setting release it. The cursor stays free while you use other windows.

The resolution list is no longer truncated to 127 entries.

The driving FOV multiplier keeps the original field of view at 1.0 and adjusts the driving camera. If your game version is incompatible with this option, the camera keeps its original behavior and the other fixes remain available.

## Display mode

**Display Mode** offers Fullscreen, Windowed with borders or desktop Borderless, plus resizable, borderless-at-render-size and stretched modes. Changes apply after a restart. After updating from an older release, choose the mode again.

## Known limits

- The frame-rate setting requires a restart. Separate limits for menus and loading are not supported.
- Mouse camera movement uses the game’s input; raw mouse input is not supported.

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `speed.exe`. Keep `Xidi.32.dll`, `SDL.XidiPlugin.32.dll` and `Xidi.ini` next to it for gamepad support (`dinput8.dll` loads Xidi).
3. Remove NFSU XtendedInput if it is installed; the fix replaces it.
4. Launch the game once to create `scripts\NFSUnderground.WidescreenFix.cfg`. With a gamepad, use **Default Settings** in the joypad controls menu once to get the modern layout.
5. Change the settings in the game's Options menu under Plugins (and Display), in FusionFixHub, or on fusionfix.io. Restart the game when an option requires it.

| Gamepad | Race | Front end |
|:---|:---|:---|
| Left stick / D-pad | Steer | Navigate |
| RT / LT | Accelerate / Brake, reverse | |
| A | E-brake | Select |
| B | Nitrous | Back |
| X | Look back | Performance |
| Y | Change camera | Default / stock |
| RB / LB | Shift up / down | Next / previous |
| View | Reset car | Customize |
| Menu | Pause | Select |
| LS / RS | Toggle HUD / - | Tutorial / Delete |
| LS + RS | | Quit |
| Right stick | Free look | |

<details>
  <summary> Contributors (click to expand) </summary>

@AeroWidescreen @xan1242

</details>

---

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br><a href="https://github.com/sponsors/xan1242"><img src="https://img.shields.io/badge/Sponsor_xan1242_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="292"></a><br></p>
