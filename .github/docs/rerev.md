<div align="center">

<img src="https://thirteenag.github.io/screens/rerev/main2.jpg" width="760" alt="Resident Evil Revelations Fusion Fix">

**Resident Evil Revelations Fusion Fix** lists every display resolution, adds ultrawide support, raises the variable frame rate cap, and offers FOV and damage-overlay options.

[Website](https://thirteenag.github.io/wfp#rerev) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ResidentEvilRevelations.FusionFix/)

</div>

---

## Fixes

- **Startup stability** — Fix a crash during startup.

- **Resolution list** — Every display mode of the monitor.
- **Ultrawide** — The HUD and menus keep their 16:9 shape; movies keep their aspect.
- **Film grain** — Keeps its size at high resolutions.
- **Camera** — No clipping into walls next to the camera at wide aspects.
- **Frame rate** — The **VARIABLE** frame rate option is capped at the chosen limit instead of 120.

## Options

- **Field of view** — Multiplier for the ordinary third-person player camera. Aiming, scripted views and camera transitions retain their original field of view.
- **Modern controls** — Free shoulder camera and camera-relative movement. Aiming turns the character toward the view. Keep walking while aiming, swinging the knife, reloading or switching weapons. Controller reload is X/Square; keyboard reload keeps its configured binding. While aiming, press A/Cross with left, right or back for the native dodge. On keyboard, use the configured quick-turn key with those directions. Back plus the quick-turn button/key turns around outside aiming. Available offline in Campaign and solo Raid.
- **Knife parry** — Swing the knife just before an ordinary Ooze's melee swing lands. A successful parry prevents that hit and staggers the enemy, allowing the normal melee follow-up. Available offline. Grabs, projectiles, explosions and boss attacks retain their normal behavior; there is no held-knife guard.
- **Skip intro** — Skip the startup intro.
- **Video mode** — Original aspect, fill or stretch.
- **Frame rate limit** — Unlimited, refresh rate or a number.
- **Window mode** — Windowed or borderless when the game's full screen option is off.
- **Damage overlay** — Can be hidden.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).

---

The two combat/control options are off by default and in the Vanilla and Recommended presets; Enhanced turns them on. Both apply while the game runs. The game already provides aim-walking and one-button knife swings. The added dodge controls use its side and back animations; a forward dodge is not available.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/ResidentEvilRevelations.FusionFix.cfg`. An old `scripts/ResidentEvilRevelations.FusionFix.ini` isn't used anymore and can be deleted.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

## Settings

**HUD aspect ratio** (on fusionfix.io and in FusionFixHub, applies while the game runs): Auto anchors equipment, objective text and the minimap to the screen edges; a ratio uses a centered box. Map icons and their background follow the same placement. Menus fit narrow screens as well as ultrawide ones; damage and pause overlays cover the full screen. Other HUD elements remain centered. There are no cutscene borders and no in-game settings menu.

## Known limits

- Cutscene-border controls, raw mouse input and full controller-icon replacement are not supported.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
