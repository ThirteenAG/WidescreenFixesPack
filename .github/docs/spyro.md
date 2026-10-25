<div align="center">

<img src="https://thirteenag.github.io/screens/spyro/main2.jpg" width="760" alt="Spyro Reignited Trilogy Widescreen Fix">

**Spyro Reignited Trilogy Widescreen Fix** corrects the aspect ratio, removes side bars, and provides FOV override and scaling options.

[Website](https://thirteenag.github.io/wfp#spyro) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SpyroReignitedTrilogy.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **Side bars** — Remove the black bars at the sides of the image.

## Options

- **FOV override** — Set the field of view in degrees.
- **FOV scale** — Scale the field of view.

---

## Settings

FusionFixHub and fusionfix.io change the settings; the first launch creates the settings file. Both apply at
once.

- **Field of view** (0.5 to 2.0, default 1.0): multiplies the corrected field of view; the same in
  every preset.
- **Field of view override** (in the settings file; default -1 = the game's): a fixed horizontal field of view
  in degrees at 16:9.

## Known limits

- The FOV multiplier and override also affect scripted cameras; separate gameplay-camera control is not supported.
- HUD aspect constraints, cinematic framing controls and intro skipping are not supported.

- Scripted views use the same camera, so the field of view multiplier changes them too.
- The side bars are removed by matching the value the game uses for them; a game update that changes it brings them
  back.
- Resolution, window modes, menus, controller support and anti-aliasing are the game's own (Unreal Engine 4 port),
  and remain controlled by the game.
- If a game update changes its camera code, the field of view controls may have no effect.
- No in-game settings menu.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game’s installation folder, preserving its directory structure. The included files belong under `Falcon/Binaries/Win64`.
3. Launch the game once to create `Falcon/Binaries/Win64/scripts/SpyroReignitedTrilogy.WidescreenFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.



<a href="https://github.com/sponsors/xan1242"><img src="https://img.shields.io/badge/Sponsor_xan1242_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="250"></a><br>

<a href="https://www.paypal.com/paypalme/xan1242"><img src="https://img.shields.io/badge/Donate_to_xan1242_on_PayPal-5c5c5c?style=for-the-badge&logo=paypal&logoColor=blue" width="250"></a><br>
