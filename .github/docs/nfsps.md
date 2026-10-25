<div align="center">

<img src="https://thirteenag.github.io/screens/nfsps/attract.jpg" width="760" alt="Need for Speed: ProStreet - Fusion Fix">

**Need for Speed: ProStreet Fusion Fix** adds proper widescreen support, corrects HUD and FOV, fits movies to the screen, adds modern gamepad support, runs smoothly at any refresh rate and resolves crashes and game-breaking bugs.

[Website](https://thirteenag.github.io/wfp#nfsps) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSProStreet.FusionFix/dllmain.cpp)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/nfsps/menu.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen menu">

- **Aspect Ratio** - corrected for all rendered elements, including ultrawide; the front end is scaled down to fit screens narrower than 16:9, and 16:10 is no longer stretched
- **HUD** - repositioned and scaled to match the active aspect ratio; **HUD Aspect Ratio** (Options > Interface, under the game's HUD options) is Auto (the whole screen, the HUD at its edges), 4:3, 16:9, 21:9, 32:9 or a custom ratio set on fusionfix.io or in FusionFixHub, applied live
- **Field of View** - corrected for widescreen, matching the Xbox 360 version; **FOV Scale** also offers the original field of view or a mathematically correct hor+ one
- **Movies** - **Movie Display** (Options > Plugins > Fusion Fix > Movies & Cutscenes, applied at once): Original Aspect shows the 16:9 movies whole with black bars, Fill Screen (the default) keeps their aspect and crops them to cover the screen, Stretch fills the screen
- **Cutscenes** - the in-engine scenes (race intros, the kings' scenes, the endings) use the widened field of view like the game; **Cutscene Borders** Pillarbox (the default) keeps them at 16:9 on wider screens, outside the front end. The PC version draws no letterbox bars (its scenes' letterbox channel only feeds the camera), so there is no Letterbox choice
- **Resolutions** - every display mode of at least 640x480 is listed in the video options, instead of a few fixed ones
- **High frame rates** - the game simulates at the frame rate limit with precisely paced frames, so it runs smoothly at any refresh rate; speech and texture animations keep their timing, and the drag race burnout grip is the same at any frame rate
- **Brake Lights** - come on with any amount of brake, not only when ABS pulsed the brake fully
- **Brightness** - the brightness setting is applied at launch, with the game's own brightness curve
- **Settings** - a crash no longer resets the game's settings on the next start
- **Xenon Effects** - sparks are drawn with additive blending, like on the Xbox 360
- **Post-Race Fix** - prevents the post-race screen from getting stuck on the Continue button
- **Title Screen** - uses its HD background
<br clear="both">

---

## Stability & Compatibility

<img src="https://thirteenag.github.io/screens/nfsps/race1.jpg" width="360" align="right" style="margin: 0 0 0 24px;" alt="Racing">

- **TrackStreamer Crash Fix** - a track section that didn't fit into the streaming pool could run the game out of memory; it now waits for space like every other section
- **FE Script Crash Fix** - `FE::Object::SetScript` skips objects whose memory was already released
- **Damage Memory Leak Fix** - the damage memory pools are recreated in the front end, so long sessions no longer run out of memory
- **Achievements and PunkBuster** - the unused Xbox 360 achievement system and PunkBuster anti-cheat are disabled; improves stability
- **LAN Fix** - LAN games use IPv4, which fixes LAN mode on newer Windows versions
- **Online and LAN races** - keep running in the background; input is blocked while the window isn't focused
- **Booster Pack Thanks** - the DLC thanks screen on new profiles is skipped
<br clear="both">

---

## New Options

<img src="https://thirteenag.github.io/screens/nfsps/race2.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Gameplay">

- **In-Game Options** - settings that belong to one of the game's options are listed next to it: Console Gamma, SMAA, Frame Rate and Display Mode under Graphics > Advanced, Shadow Resolution and Motion Blur under Graphics > World, HUD Aspect Ratio under Interface. Everything else is under one new **Plugins** entry of the Options menu (in place of Credits, and at the end of the race's pause Options, where the fix's page also has the graphics settings): it lists Fusion Fix first and then every other loaded plugin with in-game settings. Each opens its page, split into screens by the settings' pages (Graphics, Movies & Cutscenes, Camera, Controls, Interface): a **Preset** row first, the settings, and **Restore Defaults** last (accept, then accept again to confirm; it resets only the settings on the current screen). Large plugin and section lists open through numbered ranges, keeping every entry reachable. All settings are stored in `NFSProStreet.FusionFix.cfg` next to the plugin, which the fix writes with every setting and a comment above each; settings can also be changed on fusionfix.io or in FusionFixHub. An old `scripts/NFSProStreet.FusionFix.ini` isn't used anymore and can be deleted
- **Presets** - Vanilla (the original look: original FOV scale, no free look, the game's own 30 FPS limit in races, movies at their original aspect, no cutscene pillarbox, intros shown), Recommended (the defaults) or Enhanced (adds console gamma, SMAA and 4096 shadows), picked with left/right on the Preset row of the fix's page and applied with accept, then accept again to confirm; they never change window mode, language, sensitivities or controls
- **Language** - the game's language on the fix's Interface screen (after a restart), offering only the languages whose text files are installed. Korean, Chinese and Thai crash this exe and aren't offered (#665)
- **Skip Intro** - bypass startup FMVs and the title screen
- **Display Mode** - borderless, bordered, resizable, or borderless fullscreen; in every mode the picture is neither cropped nor blurred on scaled desktops
- **FPS Limit** - synced to the monitor refresh rate (the default), twice the refresh rate, or a fixed value from 30; the game itself limits races to 30 FPS
- **Gamepad Support** - Xbox, PlayStation, Switch and other controllers through [Xidi](https://github.com/samuelgr/Xidi) with the [SDL3 plugin](https://github.com/RibShark/Xidi-SDL3-Plugin): the Xbox 360 version's default controls, controller button names in the controls menu and console button icons in the front end (Xbox or PlayStation icons, detected automatically or chosen in the menu)
- **Stick Dead Zones** - separate dead zones for the left and right stick, in percent (default 10)
- **Field of View** - multiplies the driving camera field of view from 0.5 to 2.0 (default 1.0) and applies while the game runs; native angle limits still apply; menus, movies and the garage keep theirs
- **Custom Save Directory** - redirect user files to any subfolder in the game directory
- **Registry-Free Settings** - optionally saves all game settings to `settings.ini` instead of the registry
- **Shadow Resolution** - configurable dynamic shadow resolution (1024 / 2048 / 4096 / 8192 in the menu, up to 16384 on fusionfix.io or in FusionFixHub)
- **Motion Blur** - turn motion blur off without affecting the other World FX
- **Console Gamma** - a custom gamma curve that creates a more contrasted image with deeper colors, similar to how GTA IV looked on the Xbox 360 (off by default, on in Enhanced)
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect (Off or SMAA), listed next to the game's own anti-aliasing
- **AA Artifact Fix** - hides the 1px lines at the screen edges caused by MSAA (top and left, or all sides)
<br clear="both">

---

## Camera

<img src="https://thirteenag.github.io/screens/nfsps/camera.gif" width="400" align="right" style="margin: 0 0 16px 24px;" alt="Mouse look in-game">

- **Mouse/Stick Look** - free look for the chase cameras with the right stick or the mouse. Like the car camera of GTA San Andreas, the look direction stays fixed in the world while the car turns; after the return delay the camera eases back behind the car. With shifting on the right stick (as in the Xbox 360 layout), the stick only turns the camera sideways. Experience *Need for Speed: ProStreet* like never before with new camera angles. Free look, stick and mouse sensitivity, invert look, horizontal-only right stick, return delay, return speed and Field of View are on the fix's Camera screen (Options > Plugins > Fusion Fix).
<br clear="both">

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/nfsps/main1.jpg" alt="Before 16:9"> | <img src="https://thirteenag.github.io/screens/nfsps/main2.jpg" alt="After 16:9"> |
| Original 16:9 output | Correct 16:9 aspect ratio with proper scaling |

</div>

---

**Confine cursor** (on fusionfix.io and in FusionFixHub, on by default, applies while the game runs) keeps the pointer inside the game window. Switching away, moving the window, opening system menus or disabling the setting releases it.

The added settings fall back to English when Russian is selected.

**Display Mode** offers Fullscreen, Windowed with borders or desktop Borderless, plus resizable, borderless-at-render-size and stretched modes. Changes apply after a restart. After updating from an older release, choose the mode again.

Shadow Resolution can keep the game’s original quality-dependent sizes with Original (used by Vanilla), or override them after a restart.

## Known limits

- The frame-rate setting requires a restart and also selects the simulation rate. Unlimited rendering and separate limits for menus and loading are not supported.
- Mouse camera movement uses the game’s input; raw mouse input is not supported.

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `nfsps.exe`. Keep `Xidi.32.dll`, `SDL.XidiPlugin.32.dll` and `Xidi.ini` next to it for gamepad support (`dinput8.dll` loads Xidi).
3. Remove NFS XtendedInput if it is installed; the fix replaces it.
4. Launch the game once to create `scripts\NFSProStreet.FusionFix.cfg`.
5. Change the settings in the game's Options menu under Plugins (and next to the game's own options), in FusionFixHub, or on fusionfix.io. Restart the game when an option requires it.

<details>
  <summary> Contributors (click to expand) </summary>

@AeroWidescreen @xan1242

</details>

---

## Notes

Presets don't change the compatibility workarounds.

Plugins pages use numbered ranges when needed. Restore Defaults affects the settings on the current page. Setting values keep the menu's native text size; unusually long values and choice labels continue automatically every 12 seconds. An open choice list keeps its original values until it closes, then refreshes from the plugin. Selected settings show their full description beside the footer controls, with longer help continuing every 12 seconds. Changing a setting that needs a restart shows a separate acknowledgement notice. Settings that cannot change in the current state appear disabled and keep their help available.

Added menu text is shown in English when the game is set to Russian.

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br><a href="https://github.com/sponsors/xan1242"><img src="https://img.shields.io/badge/Sponsor_xan1242_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="292"></a><br></p>
