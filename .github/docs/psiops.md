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
- **Controller** — An Xbox-style controller works without setup, laid out like the console version, and prompts show its buttons while it is in use.
- **Updates** — An optional update notice in the native Plugins menu. Update checks are off by default.

## Options

Open **Options → Plugins → Psi-Ops Fusion Fix** from the title or pause menu. **Field of view factor** also appears in **Graphics**. Space selects a native menu row; Escape returns. Each list scrolls a line at a time, also with the mouse wheel, and categories open their own list. **Details** below a list of settings shows the selected option's full description, and Back returns to the row you came from.

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
| Controller buttons | Auto (Xbox) | Live |
| Frame limit | 60 FPS | Live |
| Check for updates | Off | Live |

Resolution and language can also be configured through fusionfix.io or FusionFixHub. A resolution value of 0 uses the desktop.

## Controller

The bundled Xidi (`Xidi.32.dll` and `Xidi.ini` beside `PsiOps.exe`) turns an Xbox-style controller into the game's default keys and mouse, laid out like the console version. PlayStation and Switch controllers work through Steam Input or DS4Windows.

| Button | Play | Menus |
|---|---|---|
| Left stick | Move | Select |
| Right stick | Look and aim | Cursor |
| RT / LT | Fire / telekinesis | |
| A | Action and jump | Accept |
| B | Wall move | Back |
| X / Y | Melee / lock on | Back (Y) |
| LB / RB | Mind drain / switch weapon | |
| Left / right stick click | Crouch / sniper zoom | |
| D-pad | Up: pyrokinesis, down: mind control, left: remote viewing, right: aura view | Select |
| Back / Start | Inventory / pause | Inventory / back |

While telekinesis is held, the D-pad raises and lowers the object; through the sniper scope it zooms in and out. While the controller was used last, objectives, tips and the menus' Cancel and Back show its buttons, with the console version's symbols for **Controller buttons** PlayStation; after keyboard or mouse input they show the keys again. Changed key bindings are not followed by the controller layout.

## Known limits

- If the installed font lacks an accented letter, menu text uses a readable spelling.
- The fix uses immediate presentation so the frame limiter can control pacing; the game’s VSync setting is overridden.
- Window mode and resolution changes require a restart. Alt+Enter and live window resizing are not supported; the native resolution picker keeps its legacy modes. Use the fix's resolution settings for other sizes.

## Installation

1. Extract the fix archive beside `PsiOps.exe`, preserving its folders.
2. Launch the game once to create `scripts/PsiOpsTheMindgateConspiracy.FusionFix.cfg`.
3. Configure the fix in game, on fusionfix.io or in FusionFixHub. Restart when the menu requests it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark&layout=mobile"><source media="(max-width: 768px)" srcset="https://fusionfix.io/credits/supporters.svg?layout=mobile"><source media="(prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark"><img width="100%" src="https://fusionfix.io/credits/supporters.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
