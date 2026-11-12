<div align="center">

<img src="https://thirteenag.github.io/screens/nfsu2/attract.jpg" width="760" alt="Need for Speed: Underground 2 - Fusion Fix">

**Need for Speed: Underground 2 Fusion Fix** adds proper widescreen support, corrects HUD and FOV, shows movies at their original aspect ratio, adds modern gamepad support and fixes high frame rate issues.

[Website](https://thirteenag.github.io/wfp#nfsu2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSUnderground2.FusionFix/dllmain.cpp)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/nfsu2/menu.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen menu">

- **Resolution** - native support for any display resolution, including ultrawide
- **Aspect Ratio** - corrected for all rendered elements
- **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (Auto, 4:3, 16:9, 21:9, 32:9, or custom), applied at once, including the minimap blips and panel backgrounds
- **Field of View** - corrected for widescreen; **FOV Scale** chooses between the Xbox version's field of view and a mathematically correct hor+ one

<img src="https://thirteenag.github.io/screens/nfsu2/race2.jpg" width="360" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen menu">

- **Movies** - **Movie Display** shows full screen movies whole at their original aspect ratio: Original Aspect inside the game's centred 4:3 frame, Fill Screen (the default) fitted to the screen; the game's anamorphic movies are shown at their intended aspect and HD replacement movies with square pixels; small movies in menus keep their box
- **Cutscene Borders** - the race intro scenes' black bars: Off, Letterbox (the original bars), Pillarbox (side bars that keep the scene at 16:9 on wider screens) or Both (the default). Off removes the bars and the scene fills the whole screen. **Animated Cutscene Borders** slides the bars in and out with the scene like the original; off shows them at once
- **Cutscene field of view** - the intro scenes use the player view's hor+ projection, so their framing between the bars is the original's; wider screens see more to the sides, which Pillarbox limits to 16:9
- **High frame rates** - the game runs at the correct speed above 60 FPS; tick-based gameplay, particles and the in-race slow motion and jump cameras follow real time
- **Engine audio** - engine and gear change sounds no longer play too slowly and stretched at high frame rates; no separate audio fix is needed
- **NOS trails** - correct length at any frame rate, also with the rear-view mirror on
- **Cutscenes and network races** - in-engine cutscenes are capped at 60 FPS (they break above that), LAN/online races run at up to 60 FPS instead of 30
- **Controls menu stutter** - the game no longer scans all controllers twice a second while the controls menu is open
- **Fix** for sticky steering, where the opposite direction was ignored until the wheels were centred
- **Fix** for a crash when a movie object is drawn without a movie playing
- **Stability** - fixes the random "0xAAAAAAAA" crash on multi-core CPUs at its cause (the game's memory allocator wasn't thread safe), so the game no longer needs to run on a single CPU core
- **No disc** - the game runs without a disc in the drive
<br clear="both">

---

## Notes

Menu rows keep their original font size. Longer values expand into the space between the label and the right arrow; labels shorten to fit the remaining space. Two lines below the list show the full selected label, value and description; long help continues on timed pages. Restart notices use a separate header line and also continue when needed. Letters missing from the installed font use basic Latin replacements or a visible question mark. Russian menu text uses English because the original fonts lack Cyrillic.

Menu Text Language is on fusionfix.io and in FusionFixHub, on the Compatibility page; it follows the game language by default and presets don't change it.

## New Options

<img src="https://thirteenag.github.io/screens/nfsu2/drag.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Drag racing">

- **In-Game Options** - one new category, **Plugins**, in the Options menu and in the pause menu's options. It lists the fix first, then every other loaded plugin with settings (such as Fusion Rain Droplets). Longer lists use a first-row page selector; left/right changes the page, and Back from a plugin returns to the plugin list. Each plugin opens its page: a Preset row, its settings (Graphics, Video, Camera & Gamepad; the first row picks the part shown) and Restore Defaults (resets that page's settings, asks to confirm). The plugin list is shown even when only the fix is installed. Display Mode, Field of View, HUD Aspect Ratio and Console Gamma are on the game's Display screen (Display Mode under Resolution), and its Default Settings resets them too. If a row cannot be added there, its setting remains in Plugins. A change that needs a restart shows a separate header note; longer notices continue on timed pages. All settings are stored in `NFSUnderground2.FusionFix.cfg` next to the plugin, which the fix writes with every setting and a comment above each; settings can also be changed on fusionfix.io or in FusionFixHub.
- **Skip Intro** - bypass startup FMVs, the title screen and the first successful profile-load confirmation, straight to the game menu
- **Presets** - the Preset row of the fix's page picks Vanilla, Recommended or Enhanced (left/right) and applies it on accept after a confirmation; it sets the picture, movie, cutscene, camera, frame rate limit and intro options at once; Vanilla uses the game's own 60 FPS cap, original-aspect movies and the original cutscene bars; window mode, language, sensitivities and dead zones stay as they are
- **Language** - switch between the languages installed with the game (restart needed); languages whose files are missing are not offered
- **Subtitles** - movie subtitles in every language; the PC release never showed them because of its US build region
- **Display Mode** - borderless, bordered, resizable, or borderless fullscreen; in every mode the picture is neither cropped nor blurred on scaled desktops
- **FPS Limit** - uncapped, fixed value, or synced to monitor refresh rate (including 2Ã—); applies at once
- **Gamepad Support** - Xbox, PlayStation, Switch and other controllers through [Xidi](https://github.com/samuelgr/Xidi) using XInput (PlayStation and Switch pads use Steam Input or DS4Windows): modern default controls with analog triggers, controller button names in the controls menu, console-style button icons in front end prompts, and gamepad buttons for all front end actions (Xbox or PlayStation names, Xbox in Auto, PlayStation when chosen in the menu)
- **Stick Dead Zones** - separate dead zones for the left and right stick, in percent (default 10)
- **Field of View** - multiplies the driving camera field of view from 0.5 to 2.0 (default 1.0) and applies while the game runs; native angle limits still apply
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsu2/drift.jpg" width="360" align="right" style="margin: 0 0 0 24px;" alt="Drift racing">

- **Custom Save Directory** - redirect user files to any subfolder in the game directory
- **Registry-Free Settings** - optionally saves all game settings to `settings.ini` instead of the registry
- **Rain Droplet Size** - adjusts the size of the on-screen rain droplets
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsu2/race3.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Racing">

- **Console Gamma** - a custom gamma curve that creates a more contrasted image with deeper colors, similar to how GTA IV looked on the Xbox 360; off by default, on in the Enhanced preset
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect; on by default, off in the Vanilla preset
- **AA Artifact Fix** - hides the 1px lines at the screen edges caused by MSAA (top and left, or all sides)
<br clear="both">

---

## Camera

<img src="https://thirteenag.github.io/screens/nfsu2/camera.gif" width="400" align="right" style="margin: 0 0 16px 24px;" alt="Mouse look in-game">

- **Mouse/Stick Look** - free look for the chase cameras with the right stick or the mouse. Like the car camera of GTA San Andreas, the look direction stays fixed in the world while the car turns; after the return delay the camera eases back behind the car. Experience *Need for Speed: Underground 2* like never before with new camera angles. Free look, stick and mouse sensitivity, invert look, horizontal-only right stick, return delay and return speed are on the fix's page under Plugins.
- **Drift Camera in All Races** - optional (off by default): the chase cameras behave as in drift races in every mode, so they no longer pull back when accelerating or close in when braking.

<br clear="both">

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/nfsu2/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/nfsu2/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |
| <img src="https://thirteenag.github.io/screens/nfsu2/vanilla_stretched_ultra.png" alt="Before stretched ultrawide"> | <img src="https://thirteenag.github.io/screens/nfsu2/vanilla_fixed_ultra.png" alt="After fixed ultrawide"> |
| The game would also be stretched in ultrawide | Fusion Fix makes it work with any aspect ratio |

</div>

---

## Window and focus

- **Pause When Unfocused** - on by default: the game pauses and mutes while another window is active, like the original. Turn it off to keep the game running in the background.
- **Confine Cursor** (on fusionfix.io and in FusionFixHub, on by default) keeps the pointer inside the game window while you play. Switching to another window, dragging the window, system menus and turning the setting off release it.
- **Raw Mouse Input** - free look reads the mouse without Windows pointer acceleration and without losing movement at high polling rates.
- **Check for Updates** - off by default; when on, the game asks fusionfix.io once per start whether a newer version of the fix is out and tells you in the main menu.
- The resolution list shows every mode of your monitor.

The driving FOV multiplier keeps the original field of view at 1.0 and adjusts the driving camera.

**Display Mode** offers Fullscreen, Windowed with borders or desktop Borderless, plus resizable and borderless-at-render-size modes. Changes apply after a restart. After updating from an older release, choose the mode again.

## Known limits

- Cutscenes and network races stay at 60 FPS or lower. Menus and pause screens have a separate frame rate limit; loading screens do not have their own limit.
- The EA logo movie at startup (shown only with Skip Intro off) keeps its 4:3 box in every Movie Display mode.

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `speed2.exe`. Keep `Xidi.32.dll` and `Xidi.ini` next to it for gamepad support (`dinput8.dll` loads Xidi).
3. Remove NFSU XtendedInput if it is installed; the fix replaces it.
4. Launch the game once to create `scripts\NFSUnderground2.FusionFix.cfg`. With a gamepad, use **Reset Keys To Default** in the joypad controls menu once to get the modern layout.
5. Change the settings in the game's Options menu under Plugins (and Display), in FusionFixHub, or on fusionfix.io. Restart the game when an option requires it.

| Gamepad | Race / free roam | Front end |
|:---|:---|:---|
| Left stick | Steer | Navigate |
| RT / LT | Accelerate / Brake, reverse | |
| A | E-brake | Select |
| B | Nitrous | Back |
| X | Engage (events, shops) | Customize, delete, GPS |
| Y | Change camera | Reset to default |
| RB / LB | Shift up / down | Next / previous |
| View | World map | Help |
| Menu | Pause | |
| LS / RS | Look back / Reset car | |
| LS + RS | | Quit |
| D-pad | Messages, HUD, steer | Navigate |
| Right stick | Free look | |

<details>
  <summary> Contributors (click to expand) </summary>

@AeroWidescreen

</details>

---

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
