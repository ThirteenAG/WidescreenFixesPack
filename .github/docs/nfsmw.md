<div align="center">

<img src="https://thirteenag.github.io/screens/nfsmw/attract.jpg" width="760" alt="Need for Speed: Most Wanted - Widescreen Fix">

**Need for Speed: Most Wanted Widescreen Fix** adds proper widescreen support, corrects HUD and FOV, fixes FMVs and shadows, and includes a range of quality-of-life improvements.

[Website](https://thirteenag.github.io/wfp#nfsmw) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSMostWanted.WidescreenFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/NFSMostWanted.WidescreenFix/scripts/NFSMostWanted.WidescreenFix.ini)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/nfsmw/menu.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen menu">

- **Resolution** - native support for any display resolution, including ultrawide
- **Aspect Ratio** - corrected for all rendered elements
- **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (16:9, 21:9, or custom); configurable UI scale and auto-fit mode
- **Field of View** - corrected for widescreen; optional Xbox 360 or mathematically correct hor+ scaling
- **FMVs** - fullscreen display for 16:9 with crop or stretch mode; auto-fit scaling for narrower ratios
<br clear="both">

---

## New Options

<img src="https://thirteenag.github.io/screens/nfsmw/drag.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Gameplay">

- **Skip Intro** - bypass startup FMVs
- **Windowed Mode** - borderless, bordered, resizable, or borderless fullscreen
- **FPS / Sim Rate** - configurable simulation refresh rate; sync to monitor refresh rate or double it
- **Gamepad Support** - Xbox, PlayStation, or no-icon text labels, with fixed front-end bindings
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsmw/race2.jpg" width="360" align="right" style="margin: 0 0 0 24px;" alt="Shadows">

- **Analog Deadzones** - configurable left and right stick deadzones
- **Shadow Resolution** - configurable dynamic shadow resolution (1024 / 2048 / up to 16384)
- **Shadow Fixes** - prevents shadows from disappearing in tunnels and under bridges; improved shadow LOD; auto-scaling based on aspect ratio
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsmw/race3.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Racing">

- **Shadow Map Format** - choose between D24S8, INTZ, DF16, DF24, or forced sharp shadows
- **Custom Save Directory** - redirect user files to any subfolder in the game directory
- **Registry-Free Settings** - optionally saves all game settings to `settings.ini` instead of the registry
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsmw/race4.jpg" width="360" align="right" style="margin: 0 0 0 24px;" alt="Experimental effects">

- **No Optical Drive** - allows the game to run without a disc present *(carried over from old fix)*
- **High Spec Audio** - forces 44100 Hz sample rate regardless of registry settings
- **Rain Droplet Scale** - adjusts the size of on-screen rain droplets
- **Console Gamma** – a custom gamma curve that produces a higher-contrast image with deeper colors (similar to the Xbox 360 versions of some games) and lets you select a matching gamma preset from the Xbox 360 version of NFS MW
<br clear="both">

<img src="https://thirteenag.github.io/screens/nfsmw/race5.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Racing">

- **Force GPU Vendor** - override the detected GPU vendor ID to affect renderer and video options behavior
- **Light Streaks** - re-enables the leftover light trail effect from Underground 2
- **Bleach Bypass** - re-enables the leftover enhanced contrast effect from Underground 2 *(experimental)*
<br clear="both">

---

## Camera

<img src="https://thirteenag.github.io/screens/nfsmw/camera.gif" width="400" align="right" style="margin: 0 0 16px 24px;" alt="Mouse look in-game">

- **Mouse/Stick Look** - fully functional free-look with adjustable sensitivity for both mouse and right stick. Experience *Need for Speed: Most Wanted* like never before with new camera angles. Reset timeout, return speed, and vertical axis inversion are all configurable in the `.ini` file.
<br clear="both">

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/nfsmw/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/nfsmw/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |
| <img src="https://thirteenag.github.io/screens/nfsmw/vanilla_stretched_ultra.png" alt="Before stretched ultrawide"> | <img src="https://thirteenag.github.io/screens/nfsmw/vanilla_fixed_ultra.png" alt="After fixed ultrawide"> |
| The game would also be stretched in ultrawide | Widescreen fix makes it work with any aspect ratio |

</div>

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `speed.exe`.
3. Optionally edit `NFSMostWanted.WidescreenFix.ini` to configure the available options.
4. Launch the game.

<details>
  <summary> Contributors (click to expand) </summary>

@AeroWidescreen @xan1242

</details>

---

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></a> <a href="https://ko-fi.com/thirteenag"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></a> <a href="https://paypal.me/SergeyP13"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></a> <a href="https://www.patreon.com/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></a> <a href="https://boosty.to/thirteenag"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></a><br><br><a href="https://github.com/sponsors/xan1242"><img src="https://img.shields.io/badge/Sponsor_xan1242_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="292"></a><br></p>