<div align="center">

<img src="https://thirteenag.github.io/screens/re2re3gc/main2.jpg" width="760" alt="Resident Evil 2 / Resident Evil 3 Fusion Fix (Dolphin)">

**Resident Evil 2 / Resident Evil 3 Fusion Fix (Dolphin)** adds door skipping, faster door transitions through emulator unthrottling, and Logitech RGB lighting for the GameCube versions of the games.

[Website](https://thirteenag.github.io/wfp#re2re3gc) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ResidentEvil2.RE3.Dolphin.FusionFix/dllmain.cpp)

</div>

---

## Features

- **Skip doors** — the door animation between rooms is skipped; the next room appears as soon as it has loaded.
- **Faster door transitions** — while the next room loads, Dolphin runs at full speed, so room changes finish sooner. Gameplay itself runs at normal speed.
- **RGB lighting** — [Logitech G LIGHTSYNC](https://www.logitechg.com/innovation/lightsync-rgb.html) keyboards show your condition: green for Fine, orange for Caution, red for Danger, purple when poisoned, with a heartbeat effect. Needs Logitech G HUB.

The fix recognises the USA, European and Japanese discs of Resident Evil 2 and Resident Evil 3: Nemesis; it was tested with the USA discs. RGB lighting is not available in the Japanese Resident Evil 2.

## Installation

1. Download the fix archive.
2. Extract it next to `Dolphin.exe` (64-bit), keeping the folders from the archive.
3. Start the game in Dolphin once; the settings file `scripts/ResidentEvil2.RE3.Dolphin.FusionFix.cfg` is created.

## Settings

Change the settings in FusionFixHub, on fusionfix.io or in the `.cfg` file with any text editor, then restart Dolphin. There is no settings page inside the game.

| Setting | Vanilla | Recommended (default) | Enhanced |
|---|---|---|---|
| Skip doors | Off | Off | On |
| Faster door transitions | Off | Off | On |
| RGB lighting | Off | On | On |

Setting texts are available in English, French, German, Italian and Spanish.

## Widescreen and display

Both games are drawn on fixed pre-rendered 4:3 backgrounds, so they stay 4:3 with black bars at the sides on wider screens; showing more to the sides isn't possible without cutting the picture. For a picture that isn't stretched, keep Dolphin's **Graphics › Aspect Ratio** on **Auto** and **Widescreen Hack** off. Resolution, anti-aliasing and window modes are Dolphin's own settings.

## Known limits

- No intro skipping, movie options or controller icon changes; use Dolphin's controller settings for your pad.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
