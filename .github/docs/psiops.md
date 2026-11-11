<div align="center">

<img src="https://thirteenag.github.io/screens/psiops/main2.jpg" width="760" alt="Psi-Ops: The Mindgate Conspiracy Fusion Fix">

**Psi-Ops: The Mindgate Conspiracy Fusion Fix** corrects widescreen projection, HUD placement, cutscene framing and movie playback, with native in-game settings.

[Website](https://thirteenag.github.io/wfp#psiops) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/PsiOpsTheMindgateConspiracy.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution and field of view** — Correct widescreen projection, with a live gameplay field-of-view factor and a 150-degree horizontal limit. Authored story and zoom views retain their own camera angles.
- **HUD** — Keep HUD proportions correct and choose a narrower safe area for the health, weapon and other HUD groups.
- **Cutscenes** — Preserve the original framing with borders. Turning borders off expands the view horizontally. The original sliding bars can be replaced with immediate borders.
- **Movies** — The 4:3 movies are shown whole at their own aspect, with black bars where the screen is wider or narrower.
- **Windows** — Fullscreen, Windowed and Borderless choices, exact windowed client dimensions, DPI awareness and square window corners. Windowed mode keeps the selected rendering size when opening Plugins or when another resize is requested.
- **Focus and cursor** — Optional pause while unfocused and cursor confinement while the game is active. In windowed mode, the Windows pointer follows the game’s cursor visibility.
- **Frame limiter** — A live limit from 30 to 1000 FPS, Unlimited, or the monitor's refresh rate.
- **Skip intro** — Skip publisher, middleware and legal movies; story movies still play.
- **Updates** — An optional update notice in the native Plugins menu. Update checks are off by default.

## Options

Open **Options → Plugins → Psi-Ops Fusion Fix** from the title or pause menu. **Field of view factor** also appears in **Graphics**. Space selects a native menu row; Escape returns. More and Previous move between pages, and Details shows the selected option's full description.

The page has one Preset row: Vanilla, Recommended or Enhanced. Vanilla keeps the original cutscene letterbox; Recommended and Enhanced also add pillarbox bars. Presets preserve personal choices such as resolution, window mode and language. A separate notice appears when a changed setting needs a restart; reverting the setting clears the notice.

| Setting | Default | Applies |
|---|---|---|
| HUD aspect constraint | Auto | Live |
| Cutscene borders | Both | Live |
| Animated cutscene borders | On | Live |
| Field of view factor | 1.0 | Live |
| Skip intro | On | Restart |
| Window mode | Fullscreen | Restart |
| Resolution width / height | Desktop | Restart |
| Confine cursor | On | Live |
| Pause when unfocused | On | Live |
| Frame limit | 60 FPS | Live |
| Check for updates | Off | Live |

Resolution and language can also be configured through fusionfix.io or FusionFixHub. A resolution value of 0 uses the desktop. The developer overlay opens with Ctrl+F10 where available.

## Known limits

- There is no controller support in the PC version.
- If the installed font lacks an accented letter, menu text uses a readable spelling.
- The fix uses immediate presentation so the frame limiter can control pacing; the game’s VSync setting is overridden.
- Window mode and resolution changes require a restart. Alt+Enter and live window resizing are not supported; the native resolution picker keeps its legacy modes. Use the fix's resolution settings for other sizes.

## Installation

1. Extract the fix archive beside `PsiOps.exe`, preserving its folders.
2. Launch the game once to create `scripts/PsiOpsTheMindgateConspiracy.FusionFix.cfg`.
3. Configure the fix in game, on fusionfix.io or in FusionFixHub. Restart when the menu requests it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
