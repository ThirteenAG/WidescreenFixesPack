<div align="center">

<img src="https://thirteenag.github.io/screens/nfsu/attract.jpg" width="760" alt="Need for Speed: Underground - Widescreen Fix">

**Need for Speed: Underground Widescreen Fix** adds proper widescreen support, corrects HUD and FOV, fixes FMVs, and includes a range of quality-of-life improvements.

[Website](https://thirteenag.github.io/wfp#nfsu) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSUnderground.WidescreenFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/NFSUnderground.WidescreenFix/scripts/NFSUnderground.WidescreenFix.ini)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/nfsu/menu.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen gameplay">

- **Resolution** - native support for any display resolution, including ultrawide
- **Aspect Ratio** - corrected for all rendered elements
- **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (16:9, 21:9, or custom)
- **Field of View** - corrected for widescreen; optional mathematically accurate hor+ scaling
- **FMVs** - fullscreen display for 16:9 with crop or stretch mode; auto-fit scaling for narrower ratios
- **Fix** for the **black** background in **magazine cover** screens
<br clear="both">

---

## New Options

<img src="https://thirteenag.github.io/screens/nfsu/drag.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Resolution selector in the menu">

- **Skip Intro** - bypass startup FMVs
- **Windowed Mode** - borderless, bordered, resizable, or borderless fullscreen
- **FPS Limit** - uncapped, fixed value, or synced to monitor refresh rate (including 2×)
- **Gamepad Support** - Xbox or PlayStation icons, fixed front-end bindings
- **Analog Deadzones** - configurable left and right stick deadzones

<br clear="both">
<img src="https://thirteenag.github.io/screens/nfsu/drift.jpg" width="360" align="right" style="margin: 0 24px 0px 0;" alt="Resolution selector in the menu">

- **Custom Save Directory** - redirect user files to any subfolder in game directory
- **Hide Debug Objects** - removes debug UI that is only visible in widescreen
- **Audio Sample Rate** - override the game's default 44100 Hz if needed
- **Console Gamma** - a custom gamma curve that creates a more contrasted image with deeper colors, similar to how some games looked on the Xbox 360
<br clear="both">

---

## Camera

<img src="https://thirteenag.github.io/screens/nfsu/camera.gif" width="400" align="right" style="margin: 0 0 16px 24px;" alt="Mouse look in-game">

- **Mouse/Stick Look** - fully functional free-look with adjustable sensitivity for both mouse and right stick. Experience **Need for Speed: Underground** like never before with new camera angles. Reset timeout, return speed, and vertical axis inversion are all configurable in the ini file.
<br clear="both">

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/nfsu/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/nfsu/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |
| <img src="https://thirteenag.github.io/screens/nfsu/vanilla_stretched_ultra.png" alt="Before stretched ultrawide"> | <img src="https://thirteenag.github.io/screens/nfsu/vanilla_fixed_ultra.png" alt="After fixed ultrawide"> |
| The game would also be stretched in ultrawide | Widescreen fix makes it work with any aspect ratio |

</div>

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `speed.exe`.
3. Optionally edit `NFSUnderground.WidescreenFix.ini` to configure the available options.
4. Launch the game.

<details>
  <summary> Contributors (click to expand) </summary>

@AeroWidescreen @xan1242

</details>

---

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></a> <a href="https://ko-fi.com/thirteenag"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></a> <a href="https://paypal.me/SergeyP13"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></a> <a href="https://www.patreon.com/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></a> <a href="https://boosty.to/thirteenag"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></a><br><br><a href="https://github.com/sponsors/xan1242"><img src="https://img.shields.io/badge/Sponsor_xan1242_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="292"></a><br></p>