<div align="center">

<img src="https://thirteenag.github.io/screens/thesuffering/main2.jpg" width="760" alt="The Suffering Widescreen Fix">

**The Suffering Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling, field of view and video scaling for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thesuffering) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheSuffering.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — The game's own list offers every display mode, also in windowed mode; the disc check is removed.
- **Hor+ field of view** — The view keeps the original vertical field of view at any aspect ratio; recomputed when the resolution changes.
- **HUD** — HUD and 2D elements keep a centered 4:3 layout; full-screen fades still cover the screen.
- **Cutscene borders** — The letterbox bars can be removed or kept, switched live.
- **Display scaling** — The game gets the full resolution on scaled desktops.

Applies to The Suffering and The Suffering: Ties That Bind.

## Options

In The Suffering, open **Options → Graphics** for field of view and cutscene borders, or **Options → Plugins → The Suffering** for presets and the frame rate limit. Use Back to return to the game's options.

| Setting | Values | Default | Applies |
|---|---|---|---|
| Cutscene borders | Off, Letterbox | Off | while playing |
| Field of view factor | 0.5–2.0 | 1.0 | while playing |
| Frame rate limit | Unlimited, 30–1000 FPS | 60 | after a restart |

Presets: Vanilla keeps the original letterbox; Recommended and Enhanced remove it. The field of view factor is 1.0 in every preset. Vanilla keeps the native unlimited frame rate; Recommended and Enhanced use 60 FPS.

## Known limits

- If the launcher's Display Mode list is empty, select the first Display Driver, then choose your resolution.
- Cutscene borders have no pillarbox mode; removing the letterbox shows the full frame the game renders.
- The HUD stays in a centered 4:3 box; its width cannot be adjusted.
- The startup movies cannot be skipped with this fix.
- Choose window mode in the game's own options. All fix settings are available on fusionfix.io and in FusionFixHub.
- In Ties That Bind, configure the fix on fusionfix.io or in FusionFixHub.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/TheSuffering.WidescreenFix.cfg`.
4. Configure the fix in the game's options, on fusionfix.io or in FusionFixHub. In Ties That Bind, use fusionfix.io or FusionFixHub. An old `scripts/TheSuffering.WidescreenFix.ini` isn't used anymore and can be deleted.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
