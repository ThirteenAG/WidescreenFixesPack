<div align="center">

<img src="http://thirteenag.github.io/screens/truecrimenyc/main2.jpg" width="760" alt="True Crime: New York City Widescreen Fix">

**True Crime: New York City Widescreen Fix** corrects widescreen scaling and field of view on PC, adds raw mouse input and corrected 60 FPS timing, and offers shadow and post-processing options. A separate fix for the PlayStation 2 version is described below.

[Website](http://thirteenag.github.io/wfp#truecrimenyc) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TrueCrimeNewYorkCity.WidescreenFix/dllmain.cpp)

</div>

---

The included ASI loader provides Direct3D 8 to 9 conversion. Keep `d3d8.dll` and `d3d8.ini` together beside the game executable.

## Fixes

- **Resolution** — Correct resolution handling for widescreen displays.
- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.

### Graphics options

- **Shadows** — Correct shadow rendering issues.
- **Mouse input** — Add raw mouse input support.

## Options

- **60 FPS** — Optional 60 FPS mode with corrected game speed. Gameplay above 60 FPS is not supported by this fix.
- **Windowed mode** — Enable borderless windowed mode.
- **HUD aspect ratio** — Constrain the HUD to 16:9, 21:9 or a custom aspect ratio.
- **Language** — Choose the game's language (English US/UK, French, German, Spanish, Italian, Russian) when its files are installed.
- **Raw mouse input** — Raw mouse movement for the camera and the map. **Mouse sensitivity** is a multiplier on top of the game's own sensitivity and applies while playing. **Raw mouse data** (on by default) ignores Windows mouse speed and acceleration. Turning raw input on or changing acceleration handling needs a restart.
- **Shadows** — Increase shadow resolution.
- **Depth of field** — Enable distant-object blur similar to the PS2 version. Requires d3d8to9.
- **Bloom** — Enable the bloom post-processing effect. Requires d3d8to9.
- **Anti-Aliasing** — Off, SMAA, TAA, DLAA or FSR (DLAA and FSR need Graphics API Vulkan and the files from FusionFixHub's Anti-Aliasing page). Requires d3d8to9.
- **Graphics API** — Direct3D 9, or Vulkan (DXVK) once FusionFixHub has installed it.
- **Console gamma** — Enable the Xbox 360-style gamma option. Requires d3d8to9.

The gameplay FOV multiplier ranges from 0.5 to 2.0, with 1.0 as the default. It changes normal on-foot and player-owned vehicle views. Aiming, scripted views, camera transitions, loading and pause keep their own framing.

Configure PC settings in FusionFixHub, on fusionfix.io or in `scripts/TrueCrimeNewYorkCity.WidescreenFix.cfg`. There is no added native settings menu. Supporters can also open the F10 overlay to adjust settings and view Debug Info.

## Configuration

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. Use FusionFixHub or fusionfix.io to change PC settings.

The fix generates mipmaps for compatible textures to reduce distant texture shimmer and keeps focus changes from disrupting rendering.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game. The fix creates `scripts/TrueCrimeNewYorkCity.WidescreenFix.cfg` on its first start; change the settings there or in FusionFixHub. An old `scripts/TrueCrimeNewYorkCity.WidescreenFix.ini` isn't used anymore and can be deleted.
4. Distance blur, bloom, anti-aliasing and console gamma use the ASI loader’s built-in Direct3D 8 to 9 conversion.

Credits:

- [@Parallellines0451](https://github.com/Parallellines0451) (for Xbox 360 gamma)
- [@Tervel1337](https://github.com/Tervel1337) (for bloom and depth of field)
- [@ICantReadYourMind](https://github.com/ICantReadYourMind) (for PostFX module)

## **PCSX2F: True Crime New York City Widescreen Fix**

Compatible versions:

- **SLUS-21106** (crc: **42A9C4EC**).
- **SLES-53616** (crc: **7A9D67B8**).
- **SLES-53617** (crc: **77B4F13C**).
- **SLES-53618** (crc: **B1AC3BEB**).

Installation:

- Download and extract [PCSX2 Fork With Plugins](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest).
- Download and extract plugin archive to the root directory, where exe is located.
- Change the aspect ratio or window size of the emulator.
- Launch the game.

## Fixes

- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.

## Options

- **60 FPS** — Optional 60 FPS mode with corrected game speed.
- **Skip intro** — Skip startup intro videos.

![truecrimenycpcsx2f](https://github.com/user-attachments/assets/5a34aa37-e4a4-4ceb-89c8-f3a0d49a205e)

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
