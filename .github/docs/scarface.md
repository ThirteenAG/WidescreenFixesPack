<div align="center">

<img src="https://user-images.githubusercontent.com/4904157/196225002-83a52973-0588-4c65-8019-581bffc7c1b8.gif" width="760" alt="Scarface: The World Is Yours Fusion Fix">

**Scarface: The World Is Yours Fusion Fix** corrects mouse look and framerate-dependent behavior, adds widescreen HUD placement and controller prompts, and provides native borderless window modes and optional PS2 character blood effects.

[Website](https://thirteenag.github.io/wfp#scarface) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/tree/master/source/Scarface.FusionFix) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Scarface.FusionFix/scripts/Scarface.FusionFix.ini)

</div>

---

## Fixes

- **Mouse look** — Correct mouse look behavior.
- **Gamepad** — Support controllers through [Xidi](https://github.com/samuelgr/Xidi).
- **Gamepad controls** — Use triggers to brake/accelerate and aim/shoot.
- **Controller prompts** — Show button icons according to the last keyboard, mouse, or controller input. Icons and controls-menu binding names follow the active Xidi profile, including changes while a dialog is open.
- **Weapon selection** — Scroll through weapons with the mouse wheel.
- **Windowed mode** — Use the game's native display path for borderless windows at the selected resolution or desktop resolution.
- **Widescreen HUD** — Reposition HUD elements and notifications for the chosen aspect ratio, with an option to retain the original 4:3 placement.
- **Skip intro** — Skip the legal/license screen, intro movies, and press-to-start screen.
- **Background startup** — Fix the input-acquisition loop that can hang the game when it launches without focus.
- **High framerates** — Correct the animation update's minimum frame delta, rage depletion rate, and mortar up-force decay.
- **Weapon recoil** — Correct the horizontal recoil maximum, which originally reads the minimum value.

## Options

- **Vehicle camera** — Delay automatic recentering after stick or mouse look; the default is 3 seconds. Right-mouse-button aim hold keeps its existing behavior.
- **Character blood** — Restore the PS2-style blood texture pass on characters using the game's joint damage and blood texture.
- **Console gamma** — Apply a custom gamma curve similar to GTA IV on Xbox 360.
- **Antialiasing** — Enable SMAA post-processing antialiasing; it does not work with MSAA.

Configure these settings in `scripts/Scarface.FusionFix.ini`:

| Section | Setting | Default | Behavior |
|:---|:---|:---|:---|
| `MAIN` | `SkipIntro` | `1` | Skip the legal screen, intro movies, and press-to-start screen. |
| `MAIN` | `ModernControlScheme` | `1` | Enable the Xidi profiles for modern on-foot and vehicle controls. |
| `MAIN` | `ControllerPrompts` | `1` | Switch keyboard labels and controller icons with the last input; use the bundled Xidi for current profile mappings. |
| `MAIN` | `ScrollWeaponsWithMouseWheel` | `1` | Enable mouse-wheel weapon selection. |
| `MAIN` | `VehicleCameraRecenterDelay` | `3.0` | Seconds before automatic vehicle camera recentering; `0` removes the delay. |
| `MAIN` | `WindowedMode` | `0` | `0`: fullscreen; `1`: borderless at the selected resolution; `2`: borderless at desktop resolution. |
| `MAIN` | `HudAspectRatioConstraint` | `4:3` | `Auto`: use the screen aspect ratio; `4:3`: original HUD placement; other ratios such as `16:9` are also supported. |
| `GRAPHICS` | `RestoreCharacterBlood` | `0` | Enable restored character blood effects. |
| `GRAPHICS` | `ConsoleGamma` | `0` | Enable console-style gamma. |
| `GRAPHICS` | `SMAA` | `0` | Enable SMAA; disable the game's MSAA first. |

The following fixes are enabled by default. Add their settings manually if you need to disable an individual fix:

| Section | Setting | Default |
|:---|:---|:---|
| `FRAMERATE` | `FixAnimationDelta` | `1` |
| `FRAMERATE` | `FixRageDepletion` | `1` |
| `FRAMERATE` | `FixMortarForce` | `1` |
| `FIXES` | `FixRecoilRange` | `1` |

The controls configuration screen rebinds the selected row when a controller button is pressed. Assigning a button already used by another row can leave that row **Unbound**. The controller exit prompt uses **Start**, matching the game's binding for that screen.

## Installation

1. Download and install [SilentPatch](https://github.com/CookiePLMonster/SilentPatchScarface/releases/latest). It includes required [ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader).
2. Download and unpack [Scarface.FusionFix.zip](https://github.com/ThirteenAG/WidescreenFixesPack/releases/download/scarface/Scarface.FusionFix.zip) to game's root folder.
   Keep the bundled `Xidi.32.dll` and `Xidi.ini` in the game folder so controller prompts can resolve the active profile's mappings.
3. Optionally edit `scripts/Scarface.FusionFix.ini` to configure the fix.
4. Launch the game.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
