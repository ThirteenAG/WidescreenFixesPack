<div align="center">

<img src="https://thirteenag.github.io/screens/hd2/main2.jpg" width="760" alt="Hidden &amp; Dangerous 2 Widescreen Fix">

**Hidden & Dangerous 2 Widescreen Fix** corrects widescreen scaling and field of view, with a configurable FOV multiplier. The incorrectly scaled damage overlay is disabled.

[Website](https://thirteenag.github.io/wfp#hd2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/HiddenandDangerous2.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Correct resolution handling for widescreen displays.
- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.
- **Videos** — Correct FMV scaling for widescreen.

## Settings

Open **Setup → Plugins** from the main menu on supported 1.12 versions for settings and per-plugin presets. Field of view changes while the game runs; other options require a restart. Automatic menu language uses English; select another language in FusionFixHub. The in-game font replaces accented letters with plain Latin letters. If Plugins is unavailable with your executable, use FusionFixHub.

| Setting | Values | Default |
|---|---|---|
| Correct HUD | On/off: HUD, text and cursor placement (restart) | On |
| Field of view | 0.5–2.0, applies at the next camera update | 1.0 |
| Skip intro | On/off: skips the startup logos (restart) | On |
| Missing key fallback | On/off: supplies a CD key when the game finds none; in the settings file (restart) | On |

All presets keep the HUD correction enabled. Vanilla shows the startup logos; Recommended and Enhanced skip them. Texts are available in English, French, German, Italian and Spanish.

## Known limits

- The Plugins pages can show extra empty background button outlines. Long preset labels continue on another row or page.
- The main and Setup menus have solid green side margins in widescreen.
- **Damage overlay** — disabled, because it isn't scaled properly.
- The field-of-view multiplier also applies to scripted cameras.
- No HUD aspect-ratio, window-mode or video settings.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/HiddenandDangerous2.WidescreenFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub, then restart the game. An old `scripts/HiddenandDangerous2.WidescreenFix.ini` isn't used anymore and can be deleted.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
