<div align="center">

<img src="https://thirteenag.github.io/screens/rerev2/main2.jpg" width="760" alt="Resident Evil Revelations 2 Fusion Fix">

**Resident Evil Revelations 2 Fusion Fix** adds ultrawide and fullscreen split-screen support, lists every display resolution, and fixes stuck controls before the episode 4 final boss.

[Website](https://thirteenag.github.io/wfp#rerev2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ResidentEvilRevelations2.FusionFix/)

</div>

---

## Fixes

- **Resolution list** — Every display mode of the monitor; screens narrower than 16:9 get a 16:9 picture.
- **Ultrawide** — The game renders at the screen's own aspect with the same vertical view (Hor+). Menus and other 2D screens keep their 16:9 shape in the middle of the screen.
- **HUD** — The player panel, objective, timer and messages stay where the game puts them; the minimap and the equipment panels sit at the screen's right edge. Markers on items, enemies and partners stay on their targets. Fades and damage overlays cover the whole screen.
- **Cutscenes** — On screens wider than 16:9 cutscenes keep their original 16:9 framing with black bars at the sides (can be turned off).
- **Movies** — Shown whole at their own 16:9 shape, with black bars where the screen is wider or narrower; nothing is cut off or stretched.
- **Local co-op** — Each player gets half of the screen, top and bottom or side by side.
- **Film grain** — Keeps its size at high resolutions.
- **Frame rate** — The **VARIABLE** frame rate option is capped at the chosen limit instead of 120; the fixed 30 and 60 choices keep their limits.
- **Startup** — With **Skip intro** on, the logos are skipped, and so are the notices on the way to the main menu that only offer OK: the autosave notice and the message that the RESIDENT EVIL.NET service is not available. Choices, save handling and the online sign-in itself are kept.
- **Episode 4** — Fixes controls getting stuck before the final boss battle in episode 4.

## Settings

Open **Options → Plugins → Resident Evil Revelations 2 Fusion Fix** (the last row of the Options list) for the fix's settings and presets. Left and Right change a value, the **Details** row below a setting shows its full help, **More pages** and **Previous pages** move through the settings, and **Back** returns. A notice appears when a changed setting needs a restart. `scripts/ResidentEvilRevelations2.FusionFix.cfg` is created on first start; fusionfix.io and FusionFixHub change the same settings.

| Setting | Values | Default |
|---|---|---|
| Field of view | 0.5–2.0 for the ordinary third-person camera; aiming, scripted views and cutscenes keep theirs; live | 1.0 |
| Controller icons | Auto (Xbox), Xbox, PlayStation or Nintendo button symbols; live | Auto |
| Cutscene borders | Off, Letterbox (the game's cinematic bars), Pillarbox (16:9 framing on wider screens) or Both; live | Both |
| HUD aspect ratio | Auto for the whole screen width, or 4:3, 16:9, 21:9, 32:9 or another ratio for a centered box; live | Auto |
| Frame rate limit | Unlimited, the game window's monitor refresh rate or a number; caps the game's VARIABLE option; live | 120 FPS |
| Window mode | Windowed or Borderless when the game's own Full Screen option is off; restart | Borderless |
| Pause when unfocused | On pauses in the background; off keeps the game running in windowed modes; live | On |
| Skip intro | Skips the logos and the OK-only startup notices; restart | On |
| Skip title prompt | Continues past the first press-start prompt; restart | On |
| Vertical split screen | Local co-op side by side instead of top and bottom; applies when split screen starts | Off |
| Swap co-op views | Swaps the two players' views; applies when split screen starts | Off |
| Hide damage overlay | Hides the low-health overlay; live | Off |
| Hide film grain | Hides the film grain and its vignette; restart | Off |
| Title logo | The title screen's logo: by language, Biohazard or one of the Resident Evil logos; restart | By language |
| Logitech RGB lighting | Shows both players' health on supported Logitech keyboards; restart | On |
| Check for updates | Off makes no network request; live | Off |

Vanilla shows the logos, startup notices and title prompt and keeps only the original cinematic bars; Recommended skips the startup screens and adds the cutscene side bars. Texts are available in English, French, German, Italian and Spanish.

With **Check for updates** on, the Plugins list shows a notice when a newer version is available; FusionFixHub opens after the game exits when it is installed in the game folder.

Ctrl+F10 opens the developer overlay with every setting and a Debug › Info page.

The game supports Xbox controllers natively; the fix leaves its controller input as it is. PlayStation and other controllers work through Steam Input or DS4Windows.

## Known limits

- The game's own Full Screen option uses exclusive full screen; window sizes follow the game's resolution list.
- The mouse uses the game's own input and settings.

## Installation

Extract the release archive into the game folder, beside `rerev2.exe`, preserving its folders, then start the game. Configure the fix in the game's Options, on fusionfix.io or in FusionFixHub.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
