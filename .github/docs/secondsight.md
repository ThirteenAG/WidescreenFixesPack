<div align="center">

<img src="https://thirteenag.github.io/screens/secondsight/main2.jpg" width="760" alt="Second Sight Widescreen Fix">

**Second Sight Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#secondsight) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SecondSight.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Renders at the desktop resolution, or at the resolution set in the fix's options.
- **Aspect ratio and field of view** — Correct widescreen projection with the original tuned framing.
- **Display scaling** — The game gets the full resolution on scaled desktops.

## Options

| Setting | Values | Default | Applies |
|---|---|---|---|
| Field of view factor | 0.5–2.0 | 1.0 | while playing |
| Resolution width / height | 0 = desktop | 0 | restart |

The field of view factor is 1.0 in every preset; resolution is a player choice and has no preset value.

## Known limits

- Wider views (factor above 1.0) can show through-wall rendering in some locations.
- HUD and menu width cannot be adjusted.
- The startup logos and legal screens cannot be skipped with this fix.
- No window mode setting; no in-game settings menu (fusionfix.io and FusionFixHub control the fix).
- Installing DXVK beside the game may have no effect.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/SecondSight.WidescreenFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it. An old `scripts/SecondSight.WidescreenFix.ini` isn't used anymore and can be deleted.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
