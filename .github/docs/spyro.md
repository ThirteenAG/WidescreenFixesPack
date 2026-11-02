<div align="center">

<img src="https://thirteenag.github.io/screens/spyro/main2.jpg" width="760" alt="Spyro Reignited Trilogy Fusion Fix">

**Spyro Reignited Trilogy Fusion Fix** lets the game fill screens wider than 16:9: the black bars at the sides are removed and the camera shows more to the sides instead of cutting off the top and bottom.

[Website](https://thirteenag.github.io/wfp#spyro) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SpyroReignitedTrilogy.FusionFix/dllmain.cpp)

</div>

---

## Features

- **Ultrawide picture** — on 21:9, 32:9 and other wide screens the game uses the whole screen instead of a 16:9 picture with black bars at the sides.
- **Wider view, same height** — the camera keeps the game's vertical framing and shows more to the left and right. At 16:9 the picture is exactly as in the original game.
- **Field of view** — an optional multiplier to show more or less than the game's framing.

## Installation

1. Download the fix archive.
2. Extract it into the game's folder, keeping the folders from the archive; the files go to `Falcon/Binaries/Win64`.
3. Start the game once; the settings file `Falcon/Binaries/Win64/scripts/SpyroReignitedTrilogy.FusionFix.cfg` is created.

## Settings

Change the settings in FusionFixHub, on fusionfix.io or in the `.cfg` file with any text editor. Changes apply while the game runs. There is no settings page inside the game.

| Setting | Vanilla | Recommended (default) | Enhanced |
|---|---|---|---|
| Field of view (0.5 to 2.0) | 1.0 | 1.0 | 1.0 |
| Fixed field of view (in FusionFixHub's advanced settings and the `.cfg` file) | Off | Off | Off |

- **Field of view** multiplies the view on top of the widescreen correction: 1.0 keeps the game's framing, higher values show more, lower values zoom in.
- **Fixed field of view** replaces the camera's own field of view with a fixed horizontal angle in degrees (as it would be at 16:9). Every view uses it, cutscenes and zoom effects included.

Setting texts are available in English, French, German, Italian and Spanish.

## Display and controls

Resolution, window modes, menus, HUD, controller support and anti-aliasing stay as the game handles them; the fix doesn't change them.

## Known limits

- Cutscenes and other scripted views use the same camera as gameplay, so both field of view settings change them too.
- The side bars are recognised by the size the game gives them; a game update that changes it brings them back.
- The fix is made for the current release of the game; a game update that changes its camera code can break it.

<a href="https://github.com/sponsors/xan1242"><img src="https://img.shields.io/badge/Sponsor_xan1242_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="250"></a><br>

<a href="https://www.paypal.com/paypalme/xan1242"><img src="https://img.shields.io/badge/Donate_to_xan1242_on_PayPal-5c5c5c?style=for-the-badge&logo=paypal&logoColor=blue" width="250"></a><br>
