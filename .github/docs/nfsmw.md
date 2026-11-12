<div align="center">

<img src="https://thirteenag.github.io/screens/nfsmw/attract.jpg" width="760" alt="Need for Speed: Most Wanted - Fusion Fix">

**Need for Speed: Most Wanted Fusion Fix** adds proper widescreen support, corrects HUD and FOV, shows movies at their original aspect ratio, adds modern gamepad support, adds frame pacing and selected high-frame-rate corrections and fixes shadows.

[Website](https://thirteenag.github.io/wfp#nfsmw) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSMostWanted.FusionFix/dllmain.cpp)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/nfsmw/menu.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen menu">

- **Resolution** - native support for any display resolution, including ultrawide
- **Aspect Ratio** - corrected for all rendered elements; the front end fits screens narrower than 4:3
- **HUD** - repositioned and scaled to match the active aspect ratio, anchored at the screen edges; optional constraint next to the game's video options (4:3, 16:9, 21:9, 32:9, or a custom ratio set on fusionfix.io or in FusionFixHub), applied at once when changed
- **Field of View** - corrected for widescreen, matching the Xbox 360 version; **FOV Scale** also offers the original PC field of view or a mathematically correct hor+ one
- **Movies** - shown at their correct aspect ratio (the 2:1 frames of the game's movies hold a 16:9 picture between black bars, which are left out); HD replacement movies are shown with square pixels; **Movie Display**: Original Aspect shows the whole picture inside the game's centred 4:3 frame, Fill Screen (default) fits it to the screen
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

- **In-Game Options** - display mode, field of view, HUD width and console gamma follow the game's own video options, anti-aliasing and the shadow settings its advanced video options; everything else is under one new **Plugins** category in the Options menu (also in the pause menu). It lists the fix first, then every other loaded plugin with settings (such as Fusion Rain Droplets); each opens its page with its Preset row, its settings and Restore Defaults. Values retain the game's font size; long labels fit beside them. Letters unavailable in the active game font use readable Latin replacements. Selected help appears below the list; long text continues automatically. Video settings whose rows cannot be added remain available under Plugins. The game's Video defaults reset the added settings on that page. A change that needs a restart shows the game's message once. All settings are stored in `NFSMostWanted.FusionFix.cfg` next to the plugin, which the fix writes with every setting and a comment above each; settings can also be changed on fusionfix.io or in FusionFixHub.
- **Presets** - the Preset row at the top of the fix's page picks Vanilla, Recommended or Enhanced for the picture, movie, cutscene, camera, frame rate and intro options (accept twice to apply; Vanilla is the original game with its 60 FPS cap and only the fixes); your window mode, language and control settings stay as they are
- **Language** - switch the game's language on the fix's page (after a restart); only the languages installed with the game are offered, so the game no longer crashes on a language whose files are missing
- **Cutscene Borders** - the black bars of the in-game cutscenes (race intros, pursuit and story scenes played in the game's engine): Off, Letterbox (the bars above and below of the console versions, which the PC version never drew), Pillarbox (side bars that keep the scene at 16:9 on wider screens) or Both (the default). Off shows the whole picture across the screen, like the original PC version. The race start scenes have no bars in any version, so Letterbox doesn't change them. **Animated Cutscene Borders** slides the bars in and out with the scene like the consoles; off shows them at once
- **Skip Intro** - bypass startup FMVs and the title screen, straight to the game menu
- **Underground 2 Save Bonus** - optional (off by default): a new career gets the 10000 bonus cash the game gives for an *Underground 2* save, without one in Local AppData (handy on Linux and Steam Deck)
- **Subtitles** - subtitles in the story movies and tutorials in English too; the game only showed them for the other languages
- **Display Mode** - borderless, bordered, resizable, or borderless fullscreen; in every mode the game gets the full resolution on scaled desktops
- **FPS Limit** - synced to the monitor refresh rate, twice the refresh rate, or a fixed value (restart needed)
- **Gamepad Support** - Xbox, PlayStation, Switch and other controllers through [Xidi](https://github.com/samuelgr/Xidi) using XInput (PlayStation and Switch pads use Steam Input or DS4Windows): modern default controls with independent analog triggers, controller button names in the controls menu, console button icons in the front end, and gamepad buttons for all front end actions (Xbox or PlayStation icons, Xbox in Auto, PlayStation when chosen in the menu)
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
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect; on by default, off in the Vanilla preset
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
| The game would also be stretched in ultrawide | Fusion Fix makes it work with any aspect ratio |

</div>

---

## Window and focus

- **Pause When Unfocused** - on by default: the game pauses and mutes while another window is active, like the original. Turn it off to keep the game running in the background.
- **Confine Cursor** (on fusionfix.io and in FusionFixHub, on by default) keeps the pointer inside the game window while you play. Switching to another window, dragging the window, system menus and turning the setting off release it.
- **Raw Mouse Input** - free look reads the mouse without Windows pointer acceleration and without losing movement at high polling rates.
- **Check for Updates** - off by default; when on, the game asks fusionfix.io once per start whether a newer version of the fix is out and tells you in the main menu.
- In windowed modes the window opens centered on the monitor it is on.

The driving FOV multiplier keeps the original field of view at 1.0 and adjusts the driving camera.

The added in-game settings use English when Russian is selected.

**Display Mode** offers Fullscreen, Windowed with borders or desktop Borderless, plus resizable and borderless-at-render-size modes. Changes apply after a restart. After updating from an older release, choose the mode again.

## Known limits

- The frame-rate setting requires a restart: it also sets the length of the game's simulation step, which the game's objects keep from the moment they are created. Unlimited rendering and separate limits for menus and loading are not supported.

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `speed.exe`. Keep `Xidi.32.dll` and `Xidi.ini` next to it for gamepad support (`dinput8.dll` loads Xidi).
3. Remove NFS XtendedInput if it is installed; the fix replaces it.
4. Launch the game once to create `scripts\NFSMostWanted.FusionFix.cfg`. The modern gamepad layout is set up the first time a controller is used; **Reset to defaults** in the controller settings restores it.
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

@AeroWidescreen

</details>

---

## Notes

FusionFixHub can change the settings while the game runs. Menu Text Language is on fusionfix.io and in FusionFixHub, on the Compatibility page; Automatic follows the game language. Presets don't change the compatibility settings.

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
