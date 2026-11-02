<div align="center">

<img src="https://thirteenag.github.io/screens/rerev/main2.jpg" width="760" alt="Resident Evil Revelations Fusion Fix">

**Resident Evil Revelations Fusion Fix** lists every display resolution, adds ultrawide support, makes the variable frame rate cap adjustable, and offers FOV and damage-overlay options.

[Website](https://thirteenag.github.io/wfp#rerev) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ResidentEvilRevelations.FusionFix/)

</div>

---

## Fixes

- **Resolution list** — Every display mode of the monitor.
- **Ultrawide and narrow screens** — The game renders at the screen's own aspect with the same vertical view (Hor+). Menus, the file viewer and other 2D screens keep their 16:9 shape in the middle of the screen.
- **HUD** — The objective, the minimap and the equipment panels sit at the screen edges; button prompts and messages stay centered.
- **Cutscenes** — On screens wider than 16:9 cutscenes keep their original 16:9 framing with black bars at the sides (can be turned off).
- **Movies** — Shown whole at their own aspect, with black bars where the screen is wider or narrower; nothing is cut off or stretched.
- **Film grain** — Keeps its size at high resolutions.
- **Frame rate** — The **VARIABLE** frame rate option is capped at the chosen limit instead of 120.
- **Startup** — With **Skip intro** on, the logos are skipped, and so are the notices on the way to the main menu that only offer OK: the autosave notice and the message that the RESIDENT EVIL.NET service is not available. Choices, save handling and the online sign-in itself are kept.
- **Screen saver** — The game no longer turns off the Windows screen saver setting.

## Settings

Open **Options → Plugins → Resident Evil Revelations Fusion Fix** (the last row of the Options list) for the fix's settings and presets. **More pages** and **Previous pages** move through the settings, **Details** or F1 shows the full help of the selected setting, and **Back** returns. A notice appears when a changed setting needs a restart. `scripts/ResidentEvilRevelations.FusionFix.cfg` is created on first start; fusionfix.io and FusionFixHub change the same settings.

| Setting | Values | Default |
|---|---|---|
| Field of view | 0.5–2.0 for the ordinary third-person camera; aiming, scripted views and cutscenes keep theirs; live | 1.0 |
| Cutscene borders | Off or Pillarbox; live | Pillarbox |
| Frame rate limit | Unlimited, the game window's monitor refresh rate or a number; caps the game's VARIABLE option; live | 120 FPS |
| Window mode | Windowed or Borderless when the game's own Full Screen option is off; restart | Borderless |
| Confine cursor | Keeps the mouse inside the active game window; live | On |
| Controller icons | Auto, Xbox, PlayStation or Nintendo; live | Auto (Xbox) |
| Pause when unfocused | On pauses while another window is active; off keeps the game running in windowed and borderless modes; live | On |
| Skip intro | Skips the logos and the OK-only startup notices; restart | On |
| Skip title prompt | Continues past the first press-start prompt; restart | On |
| HUD aspect ratio | Auto for the whole screen width, or 4:3, 16:9, 21:9, 32:9 or another ratio for a centered box; live | Auto |
| Hide damage overlay | Hides the low-health overlay; live | Off |
| Logitech RGB lighting | Shows the player's health on supported Logitech keyboards; restart | On |
| Modern controls | Free shoulder camera, camera-relative movement, walking while aiming, side/back dodges; offline only; live | Off |
| Knife parry | A well-timed knife swing parries ordinary Ooze melee attacks; offline only; live | Off |
| Check for updates | Off makes no network request; live | Off |

Vanilla shows the logos, startup notices and title prompt and cutscenes without side bars; Recommended skips the startup screens and adds the cutscene side bars; Enhanced keeps those choices. Modern controls and Knife parry are set separately. Texts are available in English, French, German, Italian and Spanish.

With **Check for updates** on, the Plugins list shows a notice when a newer version is available; FusionFixHub opens after the game exits when it is installed in the game folder.

Ctrl+F10 opens the developer overlay with every setting and a Debug › Info page.

The game supports Xbox controllers natively and shows their buttons.

## Known limits

- On screens wider than 16:9 the pause blur behind the file viewer and the pause menu covers only the 16:9 middle of the screen.
- The game's own Full Screen option uses exclusive full screen; window sizes follow the game's resolution list.
- PlayStation and Nintendo icon styles have not yet been verified in game; controller diagrams and other artwork may retain Xbox labels.

## Installation

Extract the release archive into the game folder, beside `rerev.exe`, preserving its folders, then start the game. Configure the fix in the game's Options, on fusionfix.io or in FusionFixHub.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
