<div align="center">

<img src="https://thirteenag.github.io/screens/psiops/main2.jpg" width="760" alt="Psi-Ops: The Mindgate Conspiracy Widescreen Fix">

**Psi-Ops: The Mindgate Conspiracy Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling, and field of view. FMV correction is incomplete.

[Website](https://thirteenag.github.io/wfp#psiops) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/PsiOpsTheMindgateConspiracy.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Renders at the desktop resolution, or at the resolution set on fusionfix.io or in FusionFixHub.
- **Aspect ratio and field of view** — Correct widescreen projection.
- **HUD** — HUD proportions and horizontal placement corrected for widescreen.
- **Skip intro** — Skips the publisher, middleware and legal movies at startup; the story intro still plays.
- **Display scaling** — The game gets the full resolution on scaled desktops.

## Options

In the game, open **Options → Plugins → Psi-Ops** for presets and the remaining fix options. **Field of view factor** is in **Graphics**, and **Correct HUD** is in **Gameplay**. Use **Cancel (Esc)** to go back. Full option descriptions are available in FusionFixHub.

| Setting | Values | Default | Applies |
|---|---|---|---|
| Correct HUD | on/off | on | restart |
| Field of view factor | 0.5–2.0 | 1.0 | while playing |
| Skip intro | on/off | on | restart |
| Resolution width / height (fusionfix.io and FusionFixHub only) | 0 = desktop | 0 | restart |

Presets: Vanilla shows the intro movies; Recommended and Enhanced skip them. HUD correction stays on and the field of view factor is 1.0 in every preset.

## Known limits

- **Videos** — Videos may not play at the correct aspect ratio [(#272)](https://github.com/ThirteenAG/WidescreenFixesPack/issues/272).
- The HUD keeps 4:3 proportions; its width cannot be adjusted.
- There is no window mode setting. All fix settings are available on fusionfix.io and in FusionFixHub.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/PsiOpsTheMindgateConspiracy.WidescreenFix.cfg`.
4. Use the in-game options above, fusionfix.io or FusionFixHub to configure the fix. Restart the game when an option requires it. An old `scripts/PsiOpsTheMindgateConspiracy.WidescreenFix.ini` isn't used anymore and can be deleted.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
