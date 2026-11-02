<div align="center">

<img src="https://thirteenag.github.io/screens/re1/main2.jpg" width="760" alt="Resident Evil Fusion Fix">

**Resident Evil Fusion Fix** expands the display resolution list, scales the TV noise filter for high resolutions, skips the start-up logos and adds a native Plugins settings page, a live frame limiter and background pause control, window modes, door skipping, cursor hiding, visual filter toggles, and Logitech RGB lighting.

[Website](https://thirteenag.github.io/wfp#re1) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ResidentEvil.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution list** — The game's resolution list offers every display mode instead of a fixed list that stopped at 2560×1600.
- **Noise filter at high resolutions** — The film grain, scanlines and rolling bar of the noise filter are drawn in screen pixels, so they shrank at 1440p and above. They are scaled by whole multiples of the screen height over 1080; 1080p and below look as before.

## Options

- **Skip intro** — Go straight to the title without the logos. The save notice that only needs OK closes by itself. The opening movie stays.
- **Skip press any key** — Continue through the first title prompt automatically. Save and load confirmations keep their normal behavior.
- **Frame rate limit** — Set **FRAME RATE** to **VARIABLE** in the game's Video settings, then choose the limit in Plugins. 60 FPS by default; Monitor follows the display refresh rate and 0 is unlimited. The game's own 30 and 60 modes keep their timing. Rates above 60 can affect gameplay.
- **Pause when unfocused** — Pause the game while another window has focus, or keep it running in the background.
- **Mouse cursor** — Hide the mouse pointer after an idle interval while the game is focused. Moving the mouse shows it again at the same place. 0 keeps the pointer visible; Always hides it while the game is focused.
- **Door animations** — Skip door, stairs and lift transitions.
- **Window mode** — Fullscreen, Windowed with the exact size chosen in the game, or Borderless at the monitor resolution. Changes apply while playing, and Alt+Enter switches between fullscreen and the last window mode. The window stays sharp at high display scaling.
- **Confine cursor** — Keep the pointer inside the focused game window. Turning it off, changing focus or opening the developer overlay releases it.
- **Controller button icons** — Xbox, PlayStation or Nintendo button pictures. Auto uses Xbox. Keyboard names stay unchanged.
- **Menu language** — The language of the fix's menu. Automatic follows the display language chosen in the game's Language options.
- **Update notice** — Optionally check for a newer fix and show a notice in Plugins. Off by default; no request is made while it is off.
- **Color correction**, **Noise filter**, **Fog** — Optionally disable each of these effects. Fog is disabled in supported rendering passes; some fog effects may remain.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/ResidentEvil.FusionFix.cfg`.
4. Open the game's **Options → Plugins → Resident Evil HD Fusion Fix** page, or configure the fix in FusionFixHub. Restart when the menu shows a restart notice.

## Settings

The **Options → Plugins** menu (from the title screen or the pause menu) and FusionFixHub change the same settings in
`scripts/ResidentEvil.FusionFix.cfg`. Use Left/Right to change a value, **F1** (or the controller's help button) to read
its full help, and Back to return. The menu works with the mouse, keyboard and controller.

Presets: **Recommended** (the default) skips the logos and the first title prompt, hides an idle cursor and limits the
frame rate to 60 FPS; door animations play as usual. **Vanilla** shows the logos and the title prompt, keeps door animations, never hides the
cursor, turns RGB lighting off and keeps the game's own 120 FPS ceiling in VARIABLE mode. **Enhanced** is Recommended
plus door skipping. The visual filter options are off in every preset, and the window mode is always your own choice.

Frame rate, background pause, cursor timeout, cursor confinement, window mode, button icons and update checks apply while
playing. Logo and title-prompt skipping, door skipping, the visual filters and RGB lighting need a restart.

**Ctrl+F10** opens the developer overlay; Debug → Info shows the fix version, build time, graphics API, resolution and
frame rate. Its supporter access uses the normal Fusion Fix account check.

## Known limits

- The game draws its pre-rendered backgrounds, characters, HUD, item and map screens and cutscenes inside its own 16:9 (Wide) or 4:3 (Original) frame and adds black bars on other screens, so nothing is stretched or cut off at any aspect ratio. A separate HUD aspect option, cutscene borders and a field-of-view setting therefore don't apply.
- Movies always play whole at their own 16:9 shape inside the game's frame, with black bars on wider or narrower screens. All of the game's movies are 16:9, so there is no separate movie option.
- The game reads controllers through XInput itself, with its own button layout and vibration settings. The icon selection changes button glyphs; other controller artwork keeps the game’s original graphics.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
