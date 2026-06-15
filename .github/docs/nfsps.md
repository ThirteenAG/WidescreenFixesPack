<div align="center">

<img src="https://thirteenag.github.io/screens/nfsps/attract.jpg" width="760" alt="Need for Speed: ProStreet - Fusion Fix">

**Need for Speed: ProStreet Fusion Fix** adds proper widescreen support, corrects HUD and FOV, fixes FMVs, resolves crashes and game-breaking bugs, and includes a range of quality-of-life improvements.

[Website](https://thirteenag.github.io/wfp#nfsps) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/NFSProStreet.FusionFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/NFSProStreet.FusionFix/scripts/NFSProStreet.FusionFix.ini)

</div>

---

## Fixes

<img src="https://thirteenag.github.io/screens/nfsps/menu.jpg" width="460" align="right" style="margin: 0 0 16px 24px;" alt="Widescreen menu">

- **Aspect Ratio** - corrected for all rendered elements, including ultrawide
- **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (16:9, 21:9, or custom); configurable UI scale and auto-fit mode
- **Field of View** - corrected for widescreen; optional Xbox 360 or mathematically correct hor+ scaling
- **FMVs** - fullscreen display with crop or stretch mode; auto-fit scaling for narrower ratios
- **Brake Lights** - fixes brake lights only activating when ABS was active
- **Gamma** - fixes wrong brightness value being applied on launch
- **Xenon Effects** - fixes alpha blending state for sparks
- **Post-Race Fix** - prevents the post-race screen from getting stuck on the Continue button
<br clear="both">

---

## Stability & Compatibility

<img src="https://thirteenag.github.io/screens/nfsps/race1.jpg" width="360" align="right" style="margin: 0 0 0 24px;" alt="Racing">

- **TrackStreamer Crash Fix** - adds memory checks to prevent crashes during track streaming
- **FE Script Crash Fix** - adds memory checks for `FE::Object::SetScript`
- **Damage Memory Leak Fix** - fixes a memory leak in damage memory pools; improves stability during long sessions
- **Disable Achievements** - disables the unused PC achievement system; improves stability
- **Disable PunkBuster** - disables PunkBuster anti-cheat; improves stability
- **Windows 11 LAN Fix** - fixes LAN mode on Windows 11 and newer
- **Disable Booster Pack Thanks** - skips the DLC thanks screen on new profiles
<br clear="both">

---

## New Options

<img src="https://thirteenag.github.io/screens/nfsps/race2.jpg" width="360" align="left" style="margin: 0 24px 0px 0;" alt="Gameplay">

- **Skip Intro** - bypass startup FMVs
- **Windowed Mode** - borderless, bordered, resizable, or borderless fullscreen
- **FPS / Sim Rate** - unlocks the frame rate; configurable simulation refresh rate; sync to monitor refresh rate or double it
- **Resolution Detection** - adds auto-detection for the in-game video options menu
- **Analog Deadzones** - configurable left and right stick deadzones
- **Custom Save Directory** - redirect user files to any subfolder in the game directory
- **Registry-Free Settings** - optionally saves all game settings to `settings.ini` instead of the registry
- **Shadow Resolution** - configurable dynamic shadow resolution
- **Motion Blur Toggle** - disable motion blur without affecting the World FX setting
- **Console Gamma** - a custom gamma curve that creates a more contrasted image with deeper colors, similar to how some games looked on the Xbox 360
- **Force FE Mode** - override the UI aspect ratio mode between widescreen and non-widescreen
<br clear="both">

---

## Camera

<img src="https://thirteenag.github.io/screens/nfsps/camera.gif" width="400" align="right" style="margin: 0 0 16px 24px;" alt="Mouse look in-game">

- **Mouse/Stick Look** - fully functional free-look with adjustable sensitivity for both mouse and right stick. Experience *Need for Speed: ProStreet* like never before with new camera angles. Reset timeout, return speed, and vertical axis inversion are all configurable in the `.ini` file.
<br clear="both">

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/nfsps/main1.jpg" alt="Before 16:9"> | <img src="https://thirteenag.github.io/screens/nfsps/main2.jpg" alt="After 16:9"> |
| Original 16:9 output | Correct 16:9 aspect ratio with proper scaling |

</div>

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `nfsps.exe`.
3. Optionally edit `NFSProStreet.FusionFix.ini` to configure the available options.
4. Launch the game.

<details>
  <summary> Contributors (click to expand) </summary>

@AeroWidescreen @xan1242

</details>

---

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><img width="100%" src="https://fusionlegacyinitiative.com/wp-content/uploads/sc-sponsors/sponsors-progress-wfp.svg"></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></a> <a href="https://ko-fi.com/thirteenag"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></a> <a href="https://paypal.me/SergeyP13"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></a> <a href="https://www.patreon.com/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></a> <a href="https://boosty.to/thirteenag"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></a><br><br><a href="https://github.com/sponsors/xan1242"><img src="https://img.shields.io/badge/Sponsor_xan1242_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="292"></a><br></p>