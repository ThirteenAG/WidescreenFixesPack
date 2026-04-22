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
- **Xbox 360 Gamma** - optional console gamma curve
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

<a href="https://github.com/sponsors/ThirteenAG"><img src="https://img.shields.io/badge/Sponsor_me_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="250"></a>&nbsp;&nbsp;<a href="https://github.com/sponsors/xan1242"><img src="https://img.shields.io/badge/Sponsor_xan1242_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="292"></a><br><a href="https://ko-fi.com/thirteenag"><img src="https://img.shields.io/badge/Support_me_on_Ko--Fi-ff5e5b?style=for-the-badge&logo=kofi&logoColor=white" width="250"></a><br><a href="https://paypal.me/SergeyP13"><img src="https://img.shields.io/badge/Donate_with_PayPal-009cde?style=for-the-badge&logo=paypal&logoColor=white" width="250"></a><br><a href="https://www.patreon.com/ThirteenAG"><img src="https://img.shields.io/badge/Support_me_on_Patreon-ff424d?style=for-the-badge&logo=patreon&logoColor=white" width="250"></a><br><a href="https://boosty.to/thirteenag/donate"><img src="https://img.shields.io/badge/Support_me_on_Boosty-f15e2d?style=for-the-badge&logo=data:image/png;base64,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&logoColor=white" width="250"></a>