<div align="center">

<img src="https://thirteenag.github.io/screens/gun/main2.jpg" width="760" alt="GUN Fusion Fix">

**GUN Fusion Fix** corrects resolution handling, aspect ratio, HUD scaling and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#gun) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Gun.FusionFix/dllmain.cpp)

</div>

---

## Settings

Open **Options → Plugins → Gun Fusion Fix** for the fix's settings and presets, on the retail and digital editions. **Display** also contains **Window Mode** and **Field of View** alongside the game's options. `scripts/Gun.FusionFix.cfg` is created on first start; fusionfix.io and FusionFixHub change the same settings.

Each list scrolls: Up and Down move through it, and the mouse wheel, Page Up/Page Down (LB/RB on a controller) and Home/End move further. Categories open their own list, and **Back** returns to the row you came from. The selected setting's description is shown under the list. Presets and Restore Defaults apply only to the selected plugin. Display settings keep their values when that plugin page is reset. A restart notice appears when a changed option needs the game to restart.

| Setting | Values | Default |
|---|---|---|
| Resolution width and height | 0 = desktop resolution, or a size; Hub/settings file; restart | 0 |
| Window mode | Fullscreen, Windowed or Borderless; live; Alt+Enter toggles fullscreen and the last window mode | Fullscreen |
| Field of view | 0.5–2.0; larger values widen the gameplay camera view, up to 150° across; live | 1.0 |
| Cutscene borders | Off, Letterbox, Pillarbox or Both; live | Both |
| HUD aspect ratio | Auto for the whole screen width, or 4:3, 16:9, 21:9, 32:9 or a custom ratio; live | Auto |
| Video mode | Original aspect shows the whole frame with its black bars; Fill enlarges the movie past the black bars around the picture until the picture fits the screen, nothing is cut off; live | Fill |
| Pause when unfocused | On pauses while another window is active; live | On |
| Confine cursor | On keeps the cursor inside the active game window; live | On |
| Gamepad icons | Automatic (Xbox), Xbox, PlayStation or Nintendo button pictures in prompts while the controller is in use; live | Automatic |
| Skip intro | Skip publisher and studio logos while preserving the game's intro; restart | On |
| Check for updates | Off makes no network request; live | Off |

Vanilla shows the logos, keeps the game's own cutscene letterbox and shows videos at their original aspect; Recommended and Enhanced skip the logos, keep the original cutscene frame with borders and fill the screen with video. Texts are available in English, French, German, Italian and Spanish. The in-game menu follows the game's language.

Cutscenes keep their original 16:9 frame: on narrower screens the game's own bars appear above and below, on wider screens the frame fills the height with bars at the sides. With the borders off, cutscenes fill the screen. The field-of-view multiplier does not change cutscenes.

In windowed and borderless modes the window keeps its exact size and is not minimized when another window becomes active; with **Pause when unfocused** off the game keeps running in the background. Fullscreen always pauses on focus loss.

With **Check for updates** on, a notice appears in the Plugins page when a newer version is available; FusionFixHub opens after the game exits when it is installed in the game folder.

The HUD keeps its proportions. With **HUD aspect ratio** on Auto, the weapon and health panel sits at the left edge of the screen and the map and instructions at the right edge; a ratio keeps them in a centered box of that shape. Objectives, messages, dialogue and menus stay centered.

Controllers: the game reads gamepads through DirectInput; the bundled Xidi presents an Xbox 360 controller to it, and the game's own Xbox 360 control layout is used out of the box. **Options → Controls** changes the bindings. While the controller is in use, prompts show its buttons; using the keyboard or moving the mouse brings back the key names. A prompt changes when the game shows it again.

## Known limits

- The game runs and renders at its own fixed 30 frames per second.
- The gameplay FOV multiplier leaves native zoom, active cutscenes and unavailable camera state unchanged.
- Long labels and descriptions continue on the next lines. The menu remains translucent over the title scene.

## Installation

Extract the release archive into the game directory, preserving its folders, then launch the game. Configure the fix on fusionfix.io or in FusionFixHub.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark&layout=mobile"><source media="(max-width: 768px)" srcset="https://fusionfix.io/credits/supporters.svg?layout=mobile"><source media="(prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark"><img width="100%" src="https://fusionfix.io/credits/supporters.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
