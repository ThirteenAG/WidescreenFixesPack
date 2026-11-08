<div align="center">

<img src="https://thirteenag.github.io/screens/nfsu/attract.jpg" width="760" alt="Need for Speed: Underground - Fusion Fix">

**Need for Speed: Underground Fusion Fix** adds proper widescreen support, corrects HUD and FOV, shows movies at their original aspect ratio, adds modern gamepad support and fixes high frame rate issues.

[Website](https://thirteenag.github.io/wfp#nfsu) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSUnderground.FusionFix/dllmain.cpp)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/nfsu/menu.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen gameplay">

- **Resolution** - native support for any display resolution, including ultrawide
- **Aspect Ratio** - corrected for all rendered elements
- **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (Auto, 4:3, 16:9, 21:9, 32:9, or custom), applied at once, including the minimap, panel backgrounds and speedometer dial
- **Field of View** - corrected for widescreen; **FOV Scale** chooses between the Xbox version's field of view and a mathematically correct hor+ one
- **Movies** - **Movie Display** shows full screen movies whole at their original aspect ratio: Original Aspect inside the game's centred 4:3 frame, Fill Screen (the default) fitted to the screen; the game's anamorphic movies are shown at their intended aspect (the story movies are 16:9, the tutorials 4:3) and HD replacement movies with square pixels; small movies in menus keep their box
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

The settings of the Plugins menu show their full name, value and description in the rows below the list; long descriptions continue on the next page after a few seconds, and a change that needs a restart adds the restart warning there. The settings added to the game's Display options show their description the same way. Russian menu text needs game fonts with Cyrillic letters.

Menu Text Language (the language of the Plugins menu) is on fusionfix.io and in FusionFixHub; it follows the game language by default.

## New Options

<img src="https://thirteenag.github.io/screens/nfsu/drag.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Resolution selector in the menu">

- **In-Game Options** - one new **Plugins** entry in the Options menu and in the pause menu's options lists the Fusion Fix first, then every other plugin with settings (such as Xbox Rain Droplets), and in the main menu the game's credits. Each plugin's page has a **Preset** row (when the plugin has presets; accept asks to confirm), its settings, and **Restore Defaults** (resets only that page, accept twice); Back returns to the list. HUD Aspect Ratio, Field of View, FOV Scale, Console Gamma and Movie Display are in the game's own **Display** options, below the game's rows. When a changed setting applies after a restart, the menu shows the restart warning below the rows. All settings are stored in `NFSUnderground.FusionFix.cfg` next to the plugin, which the fix writes with every setting and a comment above each; settings can also be changed on fusionfix.io or in FusionFixHub.
- **Skip Intro** - bypass startup FMVs and the title screen, straight to the game menu
- **Presets** - Vanilla, Recommended (the defaults) and Enhanced presets in the Preset row of the fix's page set the picture, anti-aliasing, movie, camera, frame rate limit and intro options at once (accept asks to confirm); Vanilla uses the game's own 60 FPS cap, no anti-aliasing and original-aspect movies; Recommended adds SMAA; Enhanced also adds console gamma; window mode, language, sensitivities and dead zones stay as they are
- **Language** - the game's language, chosen from the languages whose text and speech files are installed; applies after a restart
- **Display Mode** - borderless, bordered, resizable, or borderless fullscreen; in every mode the picture is neither cropped nor blurred on scaled desktops
- **Frame Rate** - uncapped, a fixed limit, or the monitor's refresh rate (or twice it); applies at once; Vanilla keeps the original 60 FPS
- **Gamepad Support** - Xbox, PlayStation, Switch and other controllers through [Xidi](https://github.com/samuelgr/Xidi) using XInput (PlayStation and Switch pads use Steam Input or DS4Windows): modern default controls with analog triggers, controller button names in the controls menu, gamepad buttons for all front end actions, and button icons that follow the last device used (Xbox style in Auto, or PlayStation style chosen in the menu); the PC menu bar's quit, delete, default/stock, tutorial, performance and customize actions are on pad buttons (see the table below)
- **Stick Dead Zones** - separate dead zones for the left and right stick, in percent (default 10)
- **Field of View** - multiplies the driving camera field of view from 0.5 to 2.0 (default 1.0) and applies while the game runs; native angle limits still apply

<br clear="both">
<img src="https://thirteenag.github.io/screens/nfsu/drift.jpg" width="360" align="right" style="margin: 0 24px 0px 0;" alt="Resolution selector in the menu">

- **Custom Save Directory** - redirect user files to any subfolder in game directory
- **Console Gamma** - a custom gamma curve that creates a more contrasted image with deeper colors, similar to how GTA IV looked on the Xbox 360; off by default, on in the Enhanced preset
- **Anti-Aliasing** - SMAA (the default) or off
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
| The game would also be stretched in ultrawide | Fusion Fix makes it work with any aspect ratio |

</div>

---

## Window and focus

- **Pause When Unfocused** - on by default: the game pauses and mutes while another window is active, like the original. Turn it off to keep the game running in the background.
- **Confine Cursor** (on fusionfix.io and in FusionFixHub, on by default) keeps the pointer inside the game window while you play. Switching to another window, dragging the window, system menus and turning the setting off release it.
- **Raw Mouse Input** - free look reads the mouse without Windows pointer acceleration and without losing movement at high polling rates.
- **Check for Updates** - off by default; when on, the game asks fusionfix.io once per start whether a newer version of the fix is out and tells you in the main menu.
- The resolution list shows every mode of your monitor.

## Display mode

**Display Mode** offers Fullscreen, Windowed with borders or desktop Borderless, plus resizable and borderless-at-render-size modes. Changes apply after a restart. After updating from an older release, choose the mode again.

## Known limits

- Menus and pause screens have a separate frame rate limit; loading screens do not have their own limit.
- The race start flybys and drag intro shots have no black bars in the original game, so the fix has no Cutscene Borders option; they use the same widescreen view as the race.

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `speed.exe`. Keep `Xidi.32.dll` and `Xidi.ini` next to it for gamepad support (`dinput8.dll` loads Xidi).
3. Remove NFSU XtendedInput if it is installed; the fix replaces it.
4. Launch the game once to create `scripts\NFSUnderground.FusionFix.cfg`. With a gamepad, use **Default Settings** in the joypad controls menu once to get the modern layout.
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

@AeroWidescreen

</details>

---

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
