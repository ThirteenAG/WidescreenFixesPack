<div align="center">

<img src="https://thirteenag.github.io/screens/gun/main2.jpg" width="760" alt="GUN Widescreen Fix">

**GUN Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#gun) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Gun.WidescreenFix/dllmain.cpp)

</div>

---

## Settings

On the GOG edition, open **Options → Plugins → Gun Widescreen Fix** for the fix's settings and presets. **Display** also contains **Window Mode** and **Field of View** alongside the game's options. Other editions can use fusionfix.io or FusionFixHub. `scripts/Gun.WidescreenFix.cfg` is created on first start.

Use **More Pages** and **Previous Pages** to move through settings. **Details** describes the last selected setting; **Back** returns to its page. Presets and Restore Defaults apply only to the selected plugin. Display settings keep their values when that plugin page is reset. A restart notice appears when a changed option needs the game to restart.

| Setting | Values | Default |
|---|---|---|
| Resolution width and height | 0 = desktop resolution, or a size; Hub/settings file; restart | 0 |
| Window mode | Fullscreen, Windowed or Borderless; live; Alt+Enter toggles fullscreen and the last window mode | Fullscreen |
| Correct HUD aspect ratio | Keep HUD and text proportions and center their positions; restart | On |
| Correct field of view | Widen the view for the display aspect ratio; restart | On |
| Field of view | 0.5–2.0; larger values widen the gameplay camera view; live | 1.0 |
| Skip intro | Skip publisher and studio logos while preserving the game's intro; restart | On |
| Game speed timing | Native timing value; Hub/settings file; restart | 30 |

Vanilla shows the logos; Recommended and Enhanced skip them. Texts are available in English, French, German, Italian and Spanish. The in-game menu follows the game's language.

## Known limits

- The gameplay FOV multiplier leaves native zoom, active cutscenes and unavailable camera state unchanged.
- HUD aspect constraints, video framing modes and cutscene-border controls are unavailable.
- The native Plugins menu supports the GOG executable. Use fusionfix.io or FusionFixHub with other executables.
- Long labels and descriptions continue across lines or pages. The menu remains translucent over the title scene.

## Installation

Extract the release archive into the game directory, preserving its folders, then launch the game. Configure the fix on fusionfix.io or in FusionFixHub.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
