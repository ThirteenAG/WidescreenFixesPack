<div align="center">

<img src="https://thirteenag.github.io/screens/mafia/main2.jpg" width="760" alt="Mafia: The City of Lost Heaven Fusion Fix">

**Mafia: The City of Lost Heaven Fusion Fix** adds proper widescreen support, corrects aspect ratio, HUD, field of view and FMV playback, and includes a range of quality-of-life improvements.

[Website](https://thirteenag.github.io/wfp#mafia) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Mafia.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Aspect Ratio** - corrected for all rendered elements
- **HUD** - repositioned and scaled to match the active aspect ratio, with an option to keep the original 4:3 centered HUD
- **Field of View** - corrected for widescreen; the field of view multiplier applies while the game runs and affects normal on-foot and driving cameras while preserving scripted, aiming and cutscene views
- **FMVs** - fullscreen videos are scaled and centered at the correct aspect ratio
- **Shadow Flickering** - fixed the z-fighting of shadows
- **Map / Text / Menus** - scaled to match the active aspect ratio, with centered 4:3 or widescreen map layouts
- **Cutscenes** - restores the original 4:3 shot composition and fits its visible image to the screen; borders can be off, letterbox, pillarbox, or both, with optional sliding animations
- **UI Scaling** - radar blips, menu underlines, crosshair details and the cursor scale with resolution

---

## New Options

- **Eye Animations** - restores the original character models with moving eyes from `A2.dta`, bypassing their static-eyed replacements in `A8.dta`; loose replacement models remain supported
- **Car Scratches** - restores the original 1.0 scratch opacity on 1.1 and 1.2, while retaining native 1.0 behavior
- **Radar Map** - draws the PC city map inside the radar with a smooth circular mask, Xbox-sized car footprints and smooth speed-based zoom. Zooms out between 0 and 60 km/h (about 37 mph), doubling the visible world range; the map and blips scale together with resolution
- **Free Camera** - optional free car camera controlled by mouse or right stick. While aiming and shooting, the view rotates independently from the current orbit position, retaining native camera collision and weapon targeting. Turn it on with the **Free Camera** option; it uses the game's look bindings, sensitivity, inversion and stick deadzones. Automatic return delay and speed are configurable
- **Draw Distance** - increases the draw distance; the new distance and the skip ranges are configurable on fusionfix.io, in FusionFixHub or in the game's menu
- **FPS Limit** - changes the frame delay, after a restart; 0 keeps the original delay
- **Settings** - saves all game settings to `savegame\settings.bin` instead of the registry, fixing an issue when the settings are lost
- **Window Mode** - fullscreen, framed window or borderless; the chosen mode applies at startup. Alt+Enter switches between fullscreen and the last window mode and keeps the framed window's position. The framed window keeps the chosen resolution on high-DPI displays
- **Stick Deadzones** - configurable deadzones for the left and right stick (otherwise the camera just spins on its own)
- **Xbox 360 Gamma** - a custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/mafia/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/mafia/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |

</div>

---

## Settings and menu

The fix creates `scripts/Mafia.FusionFix.cfg` on first launch; FusionFixHub can change the settings while the game runs. Presets: **Vanilla** (the original game with only its bugs fixed: no free camera, Xbox radar, eye animations or draw distance override, original intro and cutscene bars, original 4:3 videos), **Recommended** (the defaults: SMAA, skip intro, videos filling the screen, both cutscene borders) and **Enhanced** (adds the console gamma). An old `scripts/Mafia.FusionFix.ini` isn't used anymore and can be deleted.

In game, **Options** (title menu and pause menu) has one added row, **Plugins**: it lists Mafia Fusion Fix first, then every other loaded plugin with settings. Each plugin's page starts with its Preset row (left/right to choose, Enter twice to apply), lists its settings and ends with Restore Defaults (Enter twice; resets that plugin's settings to their defaults). A page with more rows than fit continues on a numbered page. Mafia's Options have only Control and Audio pages (video settings are in Setup.exe), so all of the fix's settings are on its own page. When a changed setting applies only after a restart, the help line says so.

HUD Aspect Ratio Constraint, Widescreen HUD, Video Widescreen Mode (Original, Fill, Stretch), Cutscene Borders, Animated Cutscene Borders, Gameplay FOV Multiplier and Pause on Focus Loss apply at once; the other settings apply at the next start. Confine Cursor and Text Language are on fusionfix.io and in FusionFixHub only.

Long plugin names may be shortened in the list; select a row to read its full name in the help area.

If **Plugins** is missing from Options, configure the fix in FusionFixHub or on fusionfix.io.

## Known limits

- Draw Distance is an absolute distance in metres, rather than a multiplier.

- The frame-rate option uses a whole-millisecond frame delay and requires a restart; it is not a precise frame limiter. Zero keeps the original delay.
- Raw mouse input and selectable controller icon families are not supported.

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `Game.exe`.
3. Launch the game once to create `scripts/Mafia.FusionFix.cfg`.
4. Configure the fix on fusionfix.io, in FusionFixHub or in the game's menu, then restart the game for settings that require it.

> [!NOTE]
> **Xbox 360 Gamma** and **SMAA** require the D3D8 to D3D9 wrapper, which is bundled and enabled by default in `d3d8.ini` (`UseD3D8to9=1`).

---

## Support the project

This project is only possible because of Fusion Fix sponsors. If you would like to see further updates and improvements, consider supporting it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
