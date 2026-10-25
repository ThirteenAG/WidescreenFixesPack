<div align="center">

<img src="https://thirteenag.github.io/screens/re1/main2.jpg" width="760" alt="Resident Evil Fusion Fix">

**Resident Evil Fusion Fix** expands the display resolution list, scales the TV noise filter for high resolutions, skips the start-up logos and offers an optional FPS unlock, door skipping, cursor hiding, visual filter toggles, and Logitech RGB lighting.

[Website](https://thirteenag.github.io/wfp#re1) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ResidentEvil.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Noise filter at high resolutions** — The film grain, scanlines and rolling bar of the noise filter are drawn in screen pixels, so they shrank at 1440p and above. They are scaled by whole multiples of the screen height over 1080; 1080p and below look as before.

## Options

- **Skip intro** — Go straight to the title without the logos. The opening movie stays.
- **Frame rate** — Enable **Unlock frame rate** and set **FRAME RATE** to **VARIABLE** to raise the frame rate cap.
- **Resolution list** — Replace the predefined list, previously capped at 2560×1600, with available display resolutions.
- **Mouse cursor** — Hide the cursor shape after an idle interval while the game is focused. Movement restores the current game cursor; the pointer position is preserved. Change the timeout while playing in FusionFixHub or on fusionfix.io; 0 keeps the cursor visible.
- **Door animations** — Skip door transitions.
- **Window mode** — Windowed or borderless when the game's full screen option is off.
- **Color correction** — Optionally disable color correction.
- **Noise filter** — Optionally disable the noise filter's grain.
- **Fog** — Optionally disable fog.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).

https://github.com/ThirteenAG/WidescreenFixesPack/assets/4904157/07921b0b-3dbf-4e59-9adf-e5587c3c54dc

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/ResidentEvil.FusionFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

## Settings

FusionFixHub and fusionfix.io change the settings; the first launch creates `scripts/ResidentEvil.FusionFix.cfg`.
An old `scripts/ResidentEvil.FusionFix.ini` isn't used anymore and can be deleted. Presets: **Recommended** (the default) skips the logos; **Vanilla**
shows them and disables resolution-list expansion, door skipping, cursor hiding and RGB lighting; **Enhanced** is the same as Recommended. The filter toggles are a matter of taste and are off in every
preset: colour correction and fog work per pixel and don't depend on resolution or aspect, and the noise filter's
scaling applies with or without its toggle. The window mode, the frame rate unlock and the GPU sync workaround are the
player's choice and aren't changed by presets. Cursor timeout changes apply while playing; the other settings apply after a restart.

## Known limits

- Resolution expansion includes sizes from 640×480 and retains at most 127 unique sizes. Larger lists are not supported.
- HUD aspect controls, movie framing and cutscene-border controls are not supported.

- The game uses fixed cameras over pre-rendered backgrounds, so there is no adjustable free-camera field of view.
- The fix doesn't force a window mode: **Window mode** applies when the game's own full screen option is off.
- **Unlock frame rate** only raises the cap of the game's **VARIABLE** option; it is off by default because animations
  and timers tied to the frame rate may misbehave above 120 FPS.
- No in-game settings menu; the controls and controller icons are the game's own.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
